# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Building QGroundControl
QGroundControl uses Qt6 (6.8.3) and CMake for building:

```bash
# Configure build (adjust Qt path as needed)
~/Qt/6.8.3/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build the application
cmake --build build --config Debug

# Run QGroundControl
./build/Debug/QGroundControl
```

You can also use CMake presets for different platforms (see `cmake/presets/` directory):
- `Linux.json` - Linux builds
- `Windows.json` - Windows builds  
- `macOS.json` - macOS builds
- `Android.json` - Android builds

### Testing
Tests are built with the `QGC_BUILD_TESTING` flag and use Qt Test framework:

```bash
# Run all tests via CTest
cd build && ctest --output-on-failure

# Or use the custom target
cmake --build build --target check

# Run individual tests
./build/Debug/QGroundControl --unittest:TestName
```

Unit tests are defined in `/test/CMakeLists.txt` and include components like:
- FactSystemTestGeneric, ParameterManagerTest
- MissionControllerTest, MissionItemTest  
- VehicleLinkManagerTest, FTPManagerTest
- And many others covering core functionality

### Documentation
Documentation is built using VitePress:

```bash
# Development server
npm run docs:dev

# Build documentation  
npm run docs:build

# Preview built docs
npm run docs:preview
```

## Architecture Overview

QGroundControl is a Qt6/QML-based ground control station for MAVLink-enabled drones, supporting both PX4 and ArduPilot firmware.

### Key Components

**Core Application (`src/`)**:
- `main.cc` - Application entry point
- `QGCApplication.cc/.h` - Main application class handling initialization
- `Settings/` - Configuration management with settings groups for different subsystems

**Vehicle Communication**:
- `Vehicle/` - Core vehicle management and state
- `MAVLink/` - MAVLink protocol handling, FTP, signing
- `Comms/` - Communication links (Serial, TCP, UDP, Bluetooth, MockLink)
- `FirmwarePlugin/` - Firmware-specific implementations (PX4, APM)

**User Interface Structure**:
- `UI/MainWindow.qml` - Top-level window (start here for UI navigation)
- `FlightDisplay/` - Fly view with flight instruments and controls
- `QmlControls/` - Plan view for mission planning and editing
- `AnalyzeView/` - Analysis tools (logs, MAVLink inspector, console)
- `AutoPilotPlugins/` - Setup wizards for PX4 and ArduPilot

**Mission Planning**:
- `MissionManager/` - Mission, geofence, and rally point management
- Complex items: Survey patterns, corridor scans, structure scans
- `PlanMasterController.cc` - Coordinates all plan elements

**Critical Subsystems**:
- `FactSystem/` - Parameter management and UI binding system
- `PositionManager/` - GPS and positioning (including RTK)
- `Camera/` - Camera control and video streaming
- `Joystick/` - Input device support via SDL

### Firmware Plugin Architecture
The codebase supports multiple autopilot firmwares through a plugin system:
- `FirmwarePlugin/` base classes define common interface
- `PX4/` and `APM/` subdirectories implement firmware-specific behavior
- Each plugin provides vehicle setup components, flight modes, and command mappings

### Settings and Configuration
Settings use a hierarchical group system in `Settings/`:
- `SettingsManager` coordinates all setting groups
- Each subsystem has its own settings group (e.g., `FlightMapSettings`, `VideoSettings`)
- Settings are automatically persisted and exposed to QML

### Navigation Tips
- Start from `UI/MainWindow.qml` to understand UI hierarchy
- Use global search for UI text to find specific components
- Major views are in their own directories: `FlightDisplay/`, `QmlControls/`
- Most QML files have corresponding C++ controllers
- Parameter definitions are in `.FactMetaData.json` files

The codebase follows Qt patterns with QML for UI and C++ for business logic, making extensive use of Qt's property binding system for reactive UI updates.