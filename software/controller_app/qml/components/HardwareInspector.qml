import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#2a2a2a"
    radius: 8
    
    // Auto-size the rectangle to perfectly wrap its children
    implicitHeight: mainLayout.implicitHeight + 20
    
    // Fallback profile string from discovery
    property string profileString: typeof discoveryWorker !== "undefined" ? discoveryWorker.hardwareProfile : "Awaiting Discovery..."
    property string roverIp: typeof discoveryWorker !== "undefined" ? discoveryWorker.roverIp : "Unknown IP"
    
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8
        
        Text {
            text: "HARDWARE PROFILE"
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#555"
        }
        
        // Basic Info
        RowLayout {
            Text { text: "Profile:"; color: "#aaa"; font.pixelSize: 12; Layout.preferredWidth: 60 }
            Text { text: root.profileString !== "" ? root.profileString : "Unknown"; color: "#00E676"; font.pixelSize: 12; font.bold: true }
        }
        RowLayout {
            Text { text: "IP Addr:"; color: "#aaa"; font.pixelSize: 12; Layout.preferredWidth: 60 }
            Text { text: root.roverIp !== "" ? root.roverIp : "---"; color: "#00E676"; font.pixelSize: 12 }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#555"
            Layout.topMargin: 5
            Layout.bottomMargin: 5
        }

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "SENSOR TELEMETRY"
                color: "white"
                font.bold: true
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: (typeof mainWindow !== "undefined" && mainWindow.isConnected) ? "#00E676" : "#FF1744"
                
                SequentialAnimation on opacity {
                    running: typeof mainWindow !== "undefined" && mainWindow.isConnected
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.3; duration: 500 }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 500 }
                }
                
                // When disconnected, stop animation and keep opacity at 1.0
                onColorChanged: {
                    if (!(typeof mainWindow !== "undefined" && mainWindow.isConnected)) {
                        opacity = 1.0;
                    }
                }
            }
        }

        // Telemetry Data Grid
        GridLayout {
            columns: 2
            columnSpacing: 10
            rowSpacing: 8
            Layout.fillWidth: true
            
            Text { text: "Battery"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? (telemetryClient.batteryVoltage * mainWindow.voltageMultiplier).toFixed(1) + " V" : "---"
                color: {
                    if (!telemetryClient) return "white";
                    let v = telemetryClient.batteryVoltage * mainWindow.voltageMultiplier;
                    let limit = appSettings ? appSettings.lowBatteryWarningVolts : 11.1;
                    return (v < limit && v > 1.0) ? "#FF1744" : "white"; // Only warn if voltage is plausible (>1.0V)
                }
                font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "IMU Temp"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? telemetryClient.imuTempC.toFixed(1) + " °C" : "---"
                color: telemetryClient && telemetryClient.imuTempC > 60.0 ? "#FF1744" : "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
            
            Text { text: "Pitch"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? (telemetryClient.pitch - (typeof appSettings !== "undefined" ? appSettings.pitchOffset : 0)).toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
            
            Text { text: "Roll"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? (telemetryClient.roll - (typeof appSettings !== "undefined" ? appSettings.rollOffset : 0)).toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "Yaw (Gyro)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? telemetryClient.yaw.toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "Compass HDG"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? telemetryClient.headingCompassDeg.toFixed(1) + "°" : "---"
                color: "#00E676"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: reverseTofSensors ? "ToF L (Back)" : "ToF L (Front)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? (telemetryClient.tof1DistMm === 0 ? "OOR" : telemetryClient.tof1DistMm + " mm") : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: reverseTofSensors ? "ToF R (Back)" : "ToF R (Front)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: telemetryClient ? (telemetryClient.tof2DistMm === 0 ? "OOR" : telemetryClient.tof2DistMm + " mm") : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
