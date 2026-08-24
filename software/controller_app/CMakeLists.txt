cmake_minimum_required(VERSION 3.16)
project(RoverGroundStation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network)

set(SOURCES
    src/main.cpp
    src/network/TelemetryClient.cpp
    src/network/CommandEmitter.cpp
    src/network/DiscoveryWorker.cpp
    src/mapping/RadarPointCloud.cpp
)

qt_add_executable(RoverGroundStation ${SOURCES})

target_link_libraries(RoverGroundStation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
)

# QML files
qt_add_qml_module(RoverGroundStation
    URI "RoverControl"
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/components/VirtualJoystick.qml
        qml/components/PolarRadarCanvas.qml
        qml/components/VideoViewport.qml
        qml/components/ArtificialHorizon.qml
        qml/components/HardwareInspector.qml
        qml/theme/Theme.qml
)

install(TARGETS RoverGroundStation
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
