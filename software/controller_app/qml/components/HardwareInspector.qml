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

        Text {
            text: "SENSOR TELEMETRY"
            color: "white"
            font.bold: true
            font.pixelSize: 12
        }

        // Telemetry Data Grid
        GridLayout {
            columns: 2
            columnSpacing: 10
            rowSpacing: 8
            Layout.fillWidth: true
            
            Text { text: "Battery"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? (telemetryClient.batteryVoltage * mainWindow.voltageMultiplier).toFixed(1) + " V" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "IMU Temp"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? telemetryClient.imuTempC.toFixed(1) + " °C" : "---"
                color: telemetryClient.imuTempC > 60.0 ? "#FF1744" : "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
            
            Text { text: "Pitch"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? telemetryClient.pitch.toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
            
            Text { text: "Roll"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? telemetryClient.roll.toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "Yaw (Gyro)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? telemetryClient.yaw.toFixed(1) + "°" : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: "Compass HDG"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? telemetryClient.headingCompassDeg.toFixed(1) + "°" : "---"
                color: "#00E676"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: reverseTofSensors ? "ToF L (Back)" : "ToF L (Front)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? (telemetryClient.tof1DistMm === 0 ? "OOR" : telemetryClient.tof1DistMm + " mm") : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }

            Text { text: reverseTofSensors ? "ToF R (Back)" : "ToF R (Front)"; color: "#888"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { 
                text: typeof telemetryClient !== "undefined" ? (telemetryClient.tof2DistMm === 0 ? "OOR" : telemetryClient.tof2DistMm + " mm") : "---"
                color: "white"; font.pixelSize: 13; font.family: "Monospace"; font.bold: true
                horizontalAlignment: Text.AlignRight; Layout.alignment: Qt.AlignRight
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
