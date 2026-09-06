import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "#0a0a0a"

    property bool isConnected: false
    
    // Connect to discovery worker to dynamically set the stream URL
    Connections {
        target: typeof discoveryWorker !== "undefined" ? discoveryWorker : null
        function onCameraDiscovered(ip) {
            console.log("VideoViewport configuring stream for camera IP: " + ip)
            if (typeof videoManager !== "undefined") {
                videoManager.startStream(ip)
                root.isConnected = true
            }
        }
    }

    Component.onCompleted: {
        if (typeof discoveryWorker !== "undefined" && discoveryWorker.cameraIp !== "") {
            console.log("VideoViewport using pre-discovered camera IP: " + discoveryWorker.cameraIp)
            if (typeof videoManager !== "undefined") {
                videoManager.startStream(discoveryWorker.cameraIp)
                root.isConnected = true
            }
        }
    }

    Connections {
        target: typeof videoManager !== "undefined" ? videoManager : null
        function onFrameReceived() {
            videoFrame.source = videoManager.currentFrameBase64
        }
        function onRecordingSaved(path) {
            console.log("Video saved to: " + path)
        }
    }

    Image {
        id: videoFrame
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        visible: root.isConnected
        cache: false
        asynchronous: false // Base64 loads instantly, disabling async prevents all flickering
        
        MouseArea {
            anchors.fill: parent
            enabled: typeof videoManager !== "undefined" ? videoManager.cvPickColorActive : false
            cursorShape: enabled ? Qt.CrossCursor : Qt.ArrowCursor
            onClicked: (mouse) => {
                if (typeof videoManager !== "undefined" && videoManager.cvPickColorActive) {
                    let xRatio = mouse.x / width
                    let yRatio = mouse.y / height
                    videoManager.requestColorPick(xRatio, yRatio)
                }
            }
        }
        
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#00FF00"
            border.width: 3
            visible: typeof videoManager !== "undefined" ? videoManager.cvPickColorActive : false
        }
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            visible: typeof videoManager !== "undefined" ? videoManager.cvPickColorActive : false

            Text {
                text: "CLICK ANYWHERE ON VIDEO TO PICK TARGET COLOR"
                color: "#00FF00"
                font.bold: true
                font.pixelSize: 24
                style: Text.Outline
                styleColor: "black"
                horizontalAlignment: Text.AlignHCenter
            }
            
            Button {
                text: "CANCEL"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    if (typeof videoManager !== "undefined") {
                        videoManager.cvPickColorActive = false
                    }
                }
            }
        }
    }

    // Recording indicator and button
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 15
        width: 120
        height: 36
        color: "#88000000"
        radius: 18
        visible: root.isConnected

        Row {
            anchors.centerIn: parent
            spacing: 8
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: (typeof videoManager !== "undefined" && videoManager.isRecording) ? "#FF1744" : "#888888"
                anchors.verticalCenter: parent.verticalCenter
                
                // Pulsing animation when recording
                SequentialAnimation on opacity {
                    running: typeof videoManager !== "undefined" && videoManager.isRecording
                    loops: Animation.Infinite
                    PropertyAnimation { to: 0.2; duration: 500 }
                    PropertyAnimation { to: 1.0; duration: 500 }
                }
            }
            
            Text {
                text: (typeof videoManager !== "undefined" && videoManager.isRecording) ? "REC" : "RECORD"
                color: "white"
                font.bold: true
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (typeof videoManager !== "undefined") {
                    videoManager.toggleRecording()
                }
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: offlineColumn.implicitWidth + 40
        height: offlineColumn.implicitHeight + 30
        color: "#AA000000"
        radius: 10
        visible: !root.isConnected

        Column {
            id: offlineColumn
            anchors.centerIn: parent
            spacing: 8

            Text {
                text: "VIDEO STREAM OFFLINE" 
                color: "#FF1744"
                font.pixelSize: 18
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: "(Waiting for ESP32-CAM mDNS Discovery or Manual IP)" 
                color: "#FF5252"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
