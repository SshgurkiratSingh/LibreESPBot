import QtQuick 2.15

Rectangle {
    color: "#050505"
    
    Rectangle {
        anchors.centerIn: parent
        width: 380
        height: 110
        color: "#110000"
        border.color: "#330000"
        border.width: 1
        radius: 8
        
        Column {
            anchors.centerIn: parent
            spacing: 12
            
            Text {
                text: "• VIDEO STREAM OFFLINE"
                color: "#FF1744"
                font.pixelSize: 18
                font.bold: true
                font.letterSpacing: 2
                anchors.horizontalCenter: parent.horizontalCenter
                
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 800 }
                    NumberAnimation { to: 1.0; duration: 800 }
                }
            }
            Text {
                text: "Awaiting ESP32-CAM MJPEG/RTSP Data..."
                color: "#888888"
                font.pixelSize: 13
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
