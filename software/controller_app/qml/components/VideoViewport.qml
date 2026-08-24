import QtQuick 2.15

Rectangle {
    color: "#0a0a0a"
    
    Text {
        anchors.centerIn: parent
        text: "VIDEO STREAM OFFLINE\n(Waiting for ESP32-CAM MJPEG/RTSP Data)"
        color: "#FF1744"
        font.pixelSize: 16
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
    }
}
