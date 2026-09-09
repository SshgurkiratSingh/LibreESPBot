import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
        // Sync RGB sliders when color is picked from screen
        function onCvSettingsChanged() {
            if (typeof videoManager === "undefined") return
            rSlider.value = videoManager.cvTrackR
            gSlider.value = videoManager.cvTrackG
            bSlider.value = videoManager.cvTrackB
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
                    // Pass actual widget dimensions so C++ can account for PreserveAspectCrop
                    videoManager.requestColorPick(xRatio, yRatio, width, height)
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
                font.pixelSize: 22
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

    // ─── RGB Color Selection Panel ──────────────────────────────────────────
    Rectangle {
        id: colorPanel
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        width: 220
        height: colorPanelCol.implicitHeight + 20
        color: "#CC111111"
        radius: 10
        visible: root.isConnected && (typeof videoManager !== "undefined") &&
                 (videoManager.cvAutoFollow)

        ColumnLayout {
            id: colorPanelCol
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
            spacing: 6

            Text {
                text: "TARGET COLOR"
                color: "#00E5FF"
                font.bold: true
                font.pixelSize: 11
                font.letterSpacing: 1
            }

            // Color swatch — shows the currently tracked color
            Rectangle {
                Layout.fillWidth: true
                height: 26
                radius: 5
                color: (typeof videoManager !== "undefined")
                    ? Qt.rgba(videoManager.cvTrackR / 255,
                              videoManager.cvTrackG / 255,
                              videoManager.cvTrackB / 255, 1)
                    : "red"
                border.color: "#555"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: (typeof videoManager !== "undefined")
                          ? "R:" + videoManager.cvTrackR + " G:" + videoManager.cvTrackG + " B:" + videoManager.cvTrackB
                          : ""
                    color: "white"
                    font.pixelSize: 10
                    style: Text.Outline
                    styleColor: "black"
                }
            }

            // R slider
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Text { text: "R"; color: "#FF5252"; font.bold: true; font.pixelSize: 11; Layout.preferredWidth: 12 }
                Slider {
                    id: rSlider
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: typeof videoManager !== "undefined" ? videoManager.cvTrackR : 255
                    onMoved: colorPanel.applyRgb()
                }
            }

            // G slider
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Text { text: "G"; color: "#69F0AE"; font.bold: true; font.pixelSize: 11; Layout.preferredWidth: 12 }
                Slider {
                    id: gSlider
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: typeof videoManager !== "undefined" ? videoManager.cvTrackG : 0
                    onMoved: colorPanel.applyRgb()
                }
            }

            // B slider
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Text { text: "B"; color: "#40C4FF"; font.bold: true; font.pixelSize: 11; Layout.preferredWidth: 12 }
                Slider {
                    id: bSlider
                    Layout.fillWidth: true
                    from: 0; to: 255; stepSize: 1
                    value: typeof videoManager !== "undefined" ? videoManager.cvTrackB : 0
                    onMoved: colorPanel.applyRgb()
                }
            }

            // Buttons row
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Button {
                    text: "🎯 Pick from Screen"
                    Layout.fillWidth: true
                    font.pixelSize: 10
                    onClicked: {
                        if (typeof videoManager !== "undefined") {
                            videoManager.cvPickColorActive = true
                        }
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#00796B" : "#004D40"
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font: parent.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Preset colors row
            Text { text: "PRESETS"; color: "#888"; font.pixelSize: 9; font.letterSpacing: 1 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Repeater {
                    model: [
                        { r: 220, g: 40,  b: 40,  label: "Red"    },
                        { r: 40,  g: 180, b: 40,  label: "Green"  },
                        { r: 40,  g: 40,  b: 220, label: "Blue"   },
                        { r: 220, g: 220, b: 40,  label: "Yellow" },
                        { r: 220, g: 100, b: 40,  label: "Orange" },
                    ]
                    Rectangle {
                        width: 28; height: 22; radius: 4
                        color: Qt.rgba(modelData.r/255, modelData.g/255, modelData.b/255, 1)
                        border.color: "#555"; border.width: 1
                        MouseArea {
                            id: presetMA
                            anchors.fill: parent
                            hoverEnabled: true
                            ToolTip {
                                text: modelData.label
                                visible: presetMA.containsMouse
                            }
                            onClicked: {
                                if (typeof videoManager !== "undefined") {
                                    videoManager.setTrackColorRGB(modelData.r, modelData.g, modelData.b)
                                }
                            }
                        }
                    }
                }
            }
        }

        function applyRgb() {
            if (typeof videoManager !== "undefined") {
                videoManager.setTrackColorRGB(rSlider.value, gSlider.value, bSlider.value)
            }
        }
    }

    // ─── Recording indicator ────────────────────────────────────────────────
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

    // ─── Offline message ────────────────────────────────────────────────────
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
