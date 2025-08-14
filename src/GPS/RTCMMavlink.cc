/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RTCMMavlink.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "GPSManager.h"
#include "GPSRtk.h"
#include "GPSRTKFactGroup.h"

QGC_LOGGING_CATEGORY(RTCMMavlinkLog, "qgc.gps.rtcmmavlink")

RTCMMavlink::RTCMMavlink(QObject *parent)
    : QObject(parent)
{
    // qCDebug(RTCMMavlinkLog) << Q_FUNC_INFO << this;

    _bandwidthTimer.start();
}

RTCMMavlink::~RTCMMavlink()
{
    // qCDebug(RTCMMavlinkLog) << Q_FUNC_INFO << this;
}

void RTCMMavlink::RTCMDataUpdate(QByteArrayView data)
{
    // 计算带宽（在所有构建模式下都运行）
    _calculateBandwith(data.size());
    
#ifdef QT_DEBUG
    qCDebug(RTCMMavlinkLog) << QString("Received RTCM data: %1 bytes - %2").arg(data.size()).arg(QString(data.toByteArray().toHex()));
#endif

    mavlink_gps_rtcm_data_t gpsRtcmData{};

    static constexpr qsizetype maxMessageLength = MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN;
    if (data.size() < maxMessageLength) {
        gpsRtcmData.len = data.size();
        gpsRtcmData.flags = (_sequenceId & 0x1FU) << 3;
        (void) memcpy(&gpsRtcmData.data, data.data(), data.size());
        _sendMessageToVehicle(gpsRtcmData);
    } else {
        uint8_t fragmentId = 0;
        qsizetype start = 0;
        while (start < data.size()) {
            gpsRtcmData.flags = 0x01U; // LSB set indicates message is fragmented
            gpsRtcmData.flags |= fragmentId++ << 1; // Next 2 bits are fragment id
            gpsRtcmData.flags |= (_sequenceId & 0x1FU) << 3; // Next 5 bits are sequence id

            const qsizetype length = std::min(data.size() - start, maxMessageLength);
            gpsRtcmData.len = length;

            (void) memcpy(gpsRtcmData.data, data.constData() + start, length);
            _sendMessageToVehicle(gpsRtcmData);

            start += length;
        }
    }

    ++_sequenceId;
}

void RTCMMavlink::_sendMessageToVehicle(const mavlink_gps_rtcm_data_t &data)
{
    QmlObjectListModel* const vehicles = MultiVehicleManager::instance()->vehicles();
    qCDebug(RTCMMavlinkLog) << QString("*** SENDING RTCM TO %1 VEHICLES, DATA LEN: %2 ***").arg(vehicles->count()).arg(data.len);
    
    if (vehicles->count() == 0) {
        qCDebug(RTCMMavlinkLog) << "*** NO VEHICLES CONNECTED - RTCM DATA NOT SENT ***";
        return;
    }
    
    for (qsizetype i = 0; i < vehicles->count(); i++) {
        Vehicle* const vehicle = qobject_cast<Vehicle*>(vehicles->get(i));
        if (!vehicle) {
            qCDebug(RTCMMavlinkLog) << QString("*** VEHICLE %1 IS NULL ***").arg(i);
            continue;
        }
        
        const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            qCDebug(RTCMMavlinkLog) << QString("*** SENDING RTCM TO VEHICLE %1 VIA LINK ***").arg(i);
            mavlink_message_t message;
            (void) mavlink_msg_gps_rtcm_data_encode_chan(
                MAVLinkProtocol::instance()->getSystemId(),
                MAVLinkProtocol::getComponentId(),
                sharedLink->mavlinkChannel(),
                &message,
                &data
            );
            (void) vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
            qCDebug(RTCMMavlinkLog) << QString("*** RTCM MESSAGE SENT TO VEHICLE %1 ***").arg(i);
        } else {
            qCDebug(RTCMMavlinkLog) << QString("*** VEHICLE %1 HAS NO PRIMARY LINK ***").arg(i);
        }
    }
}

void RTCMMavlink::_calculateBandwith(qsizetype bytes)
{
    if (!_bandwidthTimer.isValid()) {
        return;
    }

    _bandwidthByteCounter += bytes;

    const qint64 elapsed = _bandwidthTimer.elapsed();
    if (elapsed > 1000) {
        // 计算每秒字节数
        const int bytesPerSecond = (_bandwidthByteCounter * 1000) / elapsed;
        
        // 更新GPSRTKFactGroup中的数据速率
        GPSRTKFactGroup* rtkFactGroup = qobject_cast<GPSRTKFactGroup*>(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
        if (rtkFactGroup) {
            rtkFactGroup->rtcmDataRate()->setRawValue(bytesPerSecond);
        }
        
        qCDebug(RTCMMavlinkLog) << QStringLiteral("RTCM bandwidth: %1 B/s (%2 kB/s)").arg(bytesPerSecond).arg(bytesPerSecond / 1024.f);
        
        (void) _bandwidthTimer.restart();
        _bandwidthByteCounter = 0;
    }
}
