import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#202020" // Professional dark grey background
    radius: 8
    border.color: "#333333"
    border.width: 1
    
    implicitHeight: mainLayout.implicitHeight + 30
    
    // Fallback profile string from discovery
    property string profileString: typeof discoveryWorker !== "undefined" ? discoveryWorker.hardwareProfile : "Awaiting Discovery..."
    property string roverIp: typeof discoveryWorker !== "undefined" ? discoveryWorker.roverIp : "Unknown IP"
    
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15
        
        // --- HEADER ---
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "SYSTEM UPLINK"
                color: "#E0E0E0" // Soft white
                font.bold: true
                font.pixelSize: 13
                font.letterSpacing: 1
                Layout.fillWidth: true
            }
            // Pulsing status dot
            Rectangle {
                width: 10; height: 10; radius: 5
                color: (typeof mainWindow !== "undefined" && mainWindow.isConnected) ? "#4CAF50" : "#F44336" // Material Green / Red
                
                SequentialAnimation on opacity {
                    running: typeof mainWindow !== "undefined" && mainWindow.isConnected
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.3; duration: 1000; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 0.3; to: 1.0; duration: 1000; easing.type: Easing.InOutSine }
                }
            }
        }
        
        // Sub-header details
        RowLayout {
            Layout.fillWidth: true
            Text { text: "ID: " + root.profileString; color: "#888888"; font.pixelSize: 11; font.family: "Monospace" }
            Item { Layout.fillWidth: true }
            Text { text: "IP: " + root.roverIp; color: "#888888"; font.pixelSize: 11; font.family: "Monospace" }
        }
        
        RowLayout {
            Layout.fillWidth: true
            property string imuStr: telemetryClient ? (telemetryClient.activeImuType === 1 ? "MPU6050" : (telemetryClient.activeImuType === 2 ? "BMI160" : "UNK")) : "UNK"
            property string magStr: telemetryClient ? (telemetryClient.activeMagType === 1 ? "QMC5883L" : (telemetryClient.activeMagType === 2 ? "HMC5883L" : (telemetryClient.activeMagType === 3 ? "LIS3MDL" : "UNK"))) : "UNK"
            Text { text: "IMU: " + parent.imuStr; color: "#888888"; font.pixelSize: 10; font.family: "Monospace" }
            Item { Layout.fillWidth: true }
            Text { text: "MAG: " + parent.magStr; color: "#888888"; font.pixelSize: 10; font.family: "Monospace" }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }
        
        // --- BATTERY & TEMP (Horizontal Segmented Bars) ---
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            // Battery
            RowLayout {
                Layout.fillWidth: true
                Text { text: "PWR"; color: "#B0B0B0"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40 }
                
                property real battVolts: telemetryClient ? (telemetryClient.batteryVoltage * mainWindow.voltageMultiplier) : 0
                property real battPct: Math.max(0, Math.min(1, (battVolts - 9.0) / (12.6 - 9.0)))
                
                Row {
                    Layout.fillWidth: true
                    spacing: 2
                    Repeater {
                        model: 10
                        Rectangle {
                            width: (parent.width - 18) / 10; height: 8; radius: 1
                            color: index < (parent.battPct * 10) ? (parent.battPct < 0.2 ? "#F44336" : (parent.battPct < 0.4 ? "#FF9800" : "#4CAF50")) : "#2A2A2A"
                        }
                    }
                }
                Text { text: parent.battVolts.toFixed(1) + "V"; color: "#E0E0E0"; font.family: "Monospace"; font.pixelSize: 12; font.bold: true }
            }
            
            // Temperature
            RowLayout {
                Layout.fillWidth: true
                Text { text: "TMP"; color: "#B0B0B0"; font.pixelSize: 11; font.bold: true; Layout.preferredWidth: 40 }
                
                property real tempC: telemetryClient ? telemetryClient.imuTempC : 0
                property real tempPct: Math.max(0, Math.min(1, tempC / 80.0))
                
                Rectangle {
                    Layout.fillWidth: true; height: 6; radius: 3; color: "#2A2A2A"
                    Rectangle {
                        width: parent.width * parent.parent.tempPct; height: parent.height; radius: 3
                        color: parent.parent.tempC > 60 ? "#F44336" : (parent.parent.tempC > 45 ? "#FF9800" : "#2196F3")
                    }
                }
                Text { text: parent.tempC.toFixed(1) + "°C"; color: "#E0E0E0"; font.family: "Monospace"; font.pixelSize: 12; font.bold: true }
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }
        
        // --- ATTITUDE INSTRUMENTS (Mini Artificial Horizon & Compass) ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            
            // Pitch/Roll Horizon
            Item {
                width: 70; height: 70
                Layout.alignment: Qt.AlignHCenter
                
                property real pitch: telemetryClient ? (telemetryClient.pitch - (typeof appSettings !== "undefined" ? appSettings.pitchOffset : 0)) : 0
                property real roll: telemetryClient ? (telemetryClient.roll - (typeof appSettings !== "undefined" ? appSettings.rollOffset : 0)) : 0
                
                // Bezel
                Rectangle {
                    anchors.fill: parent; radius: width/2
                    color: "#2C2C2C"; border.color: "#444444"; border.width: 2
                    clip: true
                    
                    // Horizon Split (Sky / Ground)
                    Rectangle {
                        width: parent.width * 2; height: parent.height * 2
                        anchors.centerIn: parent
                        rotation: parent.parent.roll
                        
                        // Translation based on pitch (approx 1px per degree)
                        transform: Translate { y: parent.parent.parent.pitch * 1.5 }
                        
                        Rectangle { width: parent.width; height: parent.height/2; color: "#1976D2" } // Sky Blue
                        Rectangle { width: parent.width; height: parent.height/2; y: parent.height/2; color: "#795548" } // Earth Brown
                        
                        // Center horizon line
                        Rectangle { width: parent.width; height: 1; color: "#FFFFFF"; y: parent.height/2 }
                    }
                    
                    // Static aircraft reticle
                    Rectangle { width: 30; height: 2; color: "#FFEB3B"; anchors.centerIn: parent } // Yellow aircraft ref
                    Rectangle { width: 2; height: 6; color: "#FFEB3B"; anchors.centerIn: parent; anchors.verticalCenterOffset: 3 }
                }
                
                Text {
                    text: "ATTITUDE"
                    color: "#B0B0B0"; font.pixelSize: 10; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.bottom; anchors.topMargin: 5
                }
            }
            
            // Compass
            Item {
                width: 70; height: 70
                Layout.alignment: Qt.AlignHCenter
                
                property real hdg: telemetryClient ? telemetryClient.headingCompassDeg : 0
                
                // Bezel
                Rectangle {
                    anchors.fill: parent; radius: width/2
                    color: "#1E1E1E"; border.color: "#444444"; border.width: 2
                    clip: true
                    
                    // Rotating compass card
                    Item {
                        anchors.fill: parent
                        rotation: -parent.parent.hdg
                        
                        Text { text: "N"; color: "#F44336"; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top; anchors.topMargin: 2 }
                        Text { text: "S"; color: "#FFFFFF"; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 2 }
                        Text { text: "E"; color: "#FFFFFF"; font.bold: true; anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 4 }
                        Text { text: "W"; color: "#FFFFFF"; font.bold: true; anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 4 }
                    }
                    
                    // Static center marker
                    Rectangle { width: 2; height: 8; color: "#FFC107"; anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top }
                }
                
                Text {
                    text: "HEADING"
                    color: "#B0B0B0"; font.pixelSize: 10; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.bottom; anchors.topMargin: 5
                }
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333333"; Layout.topMargin: 12 }
        
        // --- KINEMATICS & DRIVE ---
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            
            RowLayout {
                Layout.fillWidth: true
                Text { text: "ACCEL"; color: "#B0B0B0"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 40 }
                Item { Layout.fillWidth: true }
                Text { 
                    text: telemetryClient ? (telemetryClient.linearAccX.toFixed(2) + "  " + telemetryClient.linearAccY.toFixed(2) + "  " + telemetryClient.linearAccZ.toFixed(2)) : "0.00  0.00  0.00"
                    color: "#E0E0E0"; font.family: "Monospace"; font.pixelSize: 11; font.bold: true 
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                Text { text: "MOTORS"; color: "#B0B0B0"; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 40 }
                Item { Layout.fillWidth: true }
                Text { 
                    text: telemetryClient ? ("L: " + telemetryClient.motorLeftPwm + "  R: " + telemetryClient.motorRightPwm) : "L: 0  R: 0"
                    color: "#E0E0E0"; font.family: "Monospace"; font.pixelSize: 11; font.bold: true 
                }
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333333"; Layout.topMargin: 5 }
        
        // --- COLLISION WARNING (ToF) ---
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            
            Text { text: "PROXIMITY RADAR SENSORS"; color: "#B0B0B0"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            
            RowLayout {
                Layout.fillWidth: true
                property real leftDist: telemetryClient ? telemetryClient.tof1DistMm : 0
                property real rightDist: telemetryClient ? telemetryClient.tof2DistMm : 0
                
                // Left ToF
                Rectangle {
                    Layout.fillWidth: true; height: 28; radius: 4
                    color: parent.leftDist > 0 && parent.leftDist < 300 ? "#33FF9800" : "#252525"
                    border.color: parent.leftDist > 0 && parent.leftDist < 300 ? "#FF9800" : "#333333"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "L: " + (parent.parent.leftDist === 0 ? "OOR" : parent.parent.leftDist + "mm")
                        color: parent.parent.leftDist > 0 && parent.parent.leftDist < 300 ? "#FFB74D" : "#E0E0E0"
                        font.family: "Monospace"; font.bold: true; font.pixelSize: 12
                    }
                }
                
                // Right ToF
                Rectangle {
                    Layout.fillWidth: true; height: 28; radius: 4
                    color: parent.rightDist > 0 && parent.rightDist < 300 ? "#33FF9800" : "#252525"
                    border.color: parent.rightDist > 0 && parent.rightDist < 300 ? "#FF9800" : "#333333"
                    
                    Text {
                        anchors.centerIn: parent
                        text: "R: " + (parent.parent.rightDist === 0 ? "OOR" : parent.parent.rightDist + "mm")
                        color: parent.parent.rightDist > 0 && parent.parent.rightDist < 300 ? "#FFB74D" : "#E0E0E0"
                        font.family: "Monospace"; font.bold: true; font.pixelSize: 12
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
