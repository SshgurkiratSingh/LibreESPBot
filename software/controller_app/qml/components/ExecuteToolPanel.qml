import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#252525"
    border.color: "#444"
    border.width: 1
    radius: 10

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "🛠 EXECUTE TOOLS"
                color: "#FF9800"
                font.bold: true
                font.pixelSize: 18
                Layout.fillWidth: true
            }
            
            Button {
                text: "Close"
                onClicked: root.visible = false
            }
        }
        
        Text {
            Layout.fillWidth: true
            color: "white"
            wrapMode: Text.WordWrap
            text: "Select an automated macro to execute. These tools will temporarily take over control of the rover."
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        // Tool 1: Panoramic Shot
        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 8
            border.color: "#333"
            border.width: 1
            implicitHeight: tool1Col.height + 20
            
            ColumnLayout {
                id: tool1Col
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                
                Text {
                    text: "Panoramic Shot (360°)"
                    color: "#00E5FF"
                    font.bold: true
                    font.pixelSize: 16
                }
                
                Text {
                    Layout.fillWidth: true
                    color: "#aaa"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    text: "Automatically rotates the bot 360 degrees using the compass, stopping every " + (typeof panoramaBuilder !== "undefined" ? panoramaBuilder.stepDegrees : 30) + "° to capture a frame and stitch them together."
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Text { text: "Step Degrees:"; color: "white"; font.pixelSize: 12 }
                    ComboBox {
                        model: [30, 45, 60, 90]
                        currentIndex: 0
                        onActivated: {
                            if (typeof panoramaBuilder !== "undefined") {
                                panoramaBuilder.stepDegrees = model[currentIndex];
                            }
                        }
                    }
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Button {
                        text: (typeof panoramaBuilder !== "undefined" && panoramaBuilder.isRunning) ? "RUNNING..." : "START PANORAMA"
                        enabled: typeof panoramaBuilder !== "undefined" && !panoramaBuilder.isRunning
                        Layout.fillWidth: true
                        onClicked: {
                            if (typeof panoramaBuilder !== "undefined") {
                                panoramaBuilder.startPanorama();
                            }
                        }
                        background: Rectangle {
                            color: parent.enabled ? "#00C853" : "#555"
                            radius: 5
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    Button {
                        text: "CANCEL"
                        enabled: typeof panoramaBuilder !== "undefined" && panoramaBuilder.isRunning
                        Layout.fillWidth: true
                        onClicked: {
                            if (typeof panoramaBuilder !== "undefined") {
                                panoramaBuilder.cancelPanorama();
                            }
                        }
                        background: Rectangle {
                            color: parent.enabled ? "#D50000" : "#555"
                            radius: 5
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                
                // Progress Bar
                ProgressBar {
                    Layout.fillWidth: true
                    value: typeof panoramaBuilder !== "undefined" ? panoramaBuilder.progressPercent / 100.0 : 0
                    visible: typeof panoramaBuilder !== "undefined" && panoramaBuilder.isRunning
                }
                
                Text {
                    Layout.fillWidth: true
                    color: "#00FF00"
                    font.pixelSize: 12
                    visible: typeof panoramaBuilder !== "undefined" && panoramaBuilder.lastResultPath !== ""
                    text: "Success! Saved to:\n" + (typeof panoramaBuilder !== "undefined" ? panoramaBuilder.lastResultPath : "")
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
        
        // Tool 2: Turning Calibrator
        Rectangle {
            Layout.fillWidth: true
            color: "#1e1e1e"
            radius: 8
            border.color: "#333"
            border.width: 1
            implicitHeight: tool2Col.height + 20
            
            ColumnLayout {
                id: tool2Col
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                
                Text {
                    text: "Turn Throttle Calibrator"
                    color: "#00E5FF"
                    font.bold: true
                    font.pixelSize: 16
                }
                
                Text {
                    Layout.fillWidth: true
                    color: "#aaa"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    text: "Finds the lowest throttle required to rotate the rover on its axis. The optimal throttle will automatically be applied to the Panoramic Shot macro."
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Button {
                        text: (typeof turningCalibrator !== "undefined" && turningCalibrator.isRunning) ? "CALIBRATING..." : "START CALIBRATION"
                        enabled: typeof turningCalibrator !== "undefined" && !turningCalibrator.isRunning
                        Layout.fillWidth: true
                        onClicked: {
                            if (typeof turningCalibrator !== "undefined") {
                                turningCalibrator.startCalibration();
                            }
                        }
                        background: Rectangle {
                            color: parent.enabled ? "#2962FF" : "#555"
                            radius: 5
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    Button {
                        text: "CANCEL"
                        enabled: typeof turningCalibrator !== "undefined" && turningCalibrator.isRunning
                        Layout.fillWidth: true
                        onClicked: {
                            if (typeof turningCalibrator !== "undefined") {
                                turningCalibrator.cancelCalibration();
                            }
                        }
                        background: Rectangle {
                            color: parent.enabled ? "#D50000" : "#555"
                            radius: 5
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                
                Text {
                    Layout.fillWidth: true
                    color: (typeof turningCalibrator !== "undefined" && turningCalibrator.optimalThrottle > 0 && !turningCalibrator.isRunning) ? "#00FF00" : "#FF9800"
                    font.pixelSize: 12
                    text: typeof turningCalibrator !== "undefined" ? turningCalibrator.statusMessage : ""
                    wrapMode: Text.WordWrap
                }
            }
        }
        
        Item { Layout.fillHeight: true } // spacer
        
        Connections {
            target: typeof panoramaBuilder !== "undefined" ? panoramaBuilder : null
            function onPanoramaError(msg) {
                console.warn(msg);
            }
            function onPanoramaFinished(path) {
                console.log("Panorama generated at:", path);
            }
        }
        
        Connections {
            target: typeof turningCalibrator !== "undefined" ? turningCalibrator : null
            function onCalibrationFinished(throttleValue) {
                if (typeof panoramaBuilder !== "undefined") {
                    panoramaBuilder.turnThrottle = throttleValue;
                    console.log("Set PanoramaBuilder turn throttle to: ", throttleValue);
                }
            }
        }
    }
}
