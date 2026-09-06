cmake_minimum_required(VERSION 3.16)
project(LibreESPBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Multimedia Concurrent)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs objdetect stitching)

set(HAARCASCADE_FILE "${CMAKE_CURRENT_BINARY_DIR}/haarcascade_frontalface_default.xml")
if(NOT EXISTS "${HAARCASCADE_FILE}")
    message(STATUS "Downloading haarcascade_frontalface_default.xml...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml"
        "${HAARCASCADE_FILE}"
        STATUS download_status
    )
    list(GET download_status 0 download_code)
    if(NOT download_code EQUAL 0)
        message(WARNING "Failed to download haarcascade file!")
    endif()
endif()
set(SOURCES
    src/main.cpp
    src/network/TelemetryClient.cpp
    src/network/CommandEmitter.cpp
    src/network/DiscoveryWorker.cpp
    src/network/VideoManager.cpp
    src/mapping/RadarPointCloud.cpp
    src/core/AppSettings.cpp
    src/core/ScriptEngine.cpp
    src/core/JoystickHandler.cpp
    src/tools/PanoramaBuilder.cpp
    src/tools/TurningCalibrator.cpp
)

qt_add_executable(LibreESPBot ${SOURCES})

target_include_directories(LibreESPBot PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(LibreESPBot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Concurrent
    ${OpenCV_LIBS}
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
        qml/components/JoystickMappingPanel.qml
        qml/components/KinematicsView3D.qml
        qml/components/AviationHud.qml
        qml/components/ScriptPanel.qml
        qml/components/ExecuteToolPanel.qml
        qml/theme/Theme.qml
)

install(TARGETS LibreESPBot
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
set_target_properties(LibreESPBot PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
