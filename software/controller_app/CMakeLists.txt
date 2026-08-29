cmake_minimum_required(VERSION 3.16)
project(LibreESPBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network QuickControls2 Multimedia)

set(SOURCES
    src/main.cpp
    src/network/TelemetryClient.cpp
    src/network/CommandEmitter.cpp
    src/network/DiscoveryWorker.cpp
    src/mapping/RadarPointCloud.cpp
)

qt_add_executable(LibreESPBot ${SOURCES})

target_link_libraries(LibreESPBot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::QuickControls2
    Qt6::Multimedia
)

# QML files
qt_add_qml_module(LibreESPBot
    URI "RoverControl"
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/VirtualJoystick.qml
        qml/components/PolarRadarCanvas.qml
        qml/components/VideoViewport.qml
        qml/components/ArtificialHorizon.qml
        qml/components/HardwareInspector.qml
        qml/components/KinematicsView3D.qml
        qml/theme/Theme.qml
)

install(TARGETS LibreESPBot
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
set_target_properties(LibreESPBot PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
