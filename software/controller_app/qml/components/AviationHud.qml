import QtQuick 2.15

Item {
    id: root
    anchors.fill: parent
    clip: true

    property real pitch: 0.0 // -90 to +90
    property real roll: 0.0  // -180 to +180
    property real yaw: 0.0   // 0 to 360

    property int throttleAxis: typeof commandEmitter !== "undefined" ? commandEmitter.currentThrottle : 0
    property int rpm: typeof appSettings !== "undefined" ? appSettings.motorRpm : 300
    property int wheelSizeMm: typeof appSettings !== "undefined" ? appSettings.wheelSizeMm : 60
    property int speedMode: typeof commandEmitter !== "undefined" ? commandEmitter.currentSpeedMode : 2
    property real speedMultiplier: speedMode === 0 ? 0.15 : (speedMode === 1 ? 0.30 : (speedMode === 2 ? 0.70 : 1.0))
    
    // Convert to m/s
    // Max Speed (mm/s) = (RPM / 60) * pi * wheelSizeMm
    // Actual Speed = Max Speed * (throttleAxis / 1023.0) * speedMultiplier
    property real maxSpeedMs: ((rpm / 60.0) * Math.PI * wheelSizeMm) / 1000.0
    property real currentSpeedMs: Math.abs((throttleAxis / 1023.0) * maxSpeedMs) * speedMultiplier

    property real frontDistMm: typeof telemetryClient !== "undefined" ? telemetryClient.tof1DistMm : 0

    property string hudColor: typeof appSettings !== "undefined" ? appSettings.hudColor : "#00E5FF"
    property real hudOpacity: typeof appSettings !== "undefined" ? appSettings.hudOpacity : 0.8

    opacity: hudOpacity

    // 1. PITCH LADDER
    Item {
        id: pitchLadder
        anchors.centerIn: parent
        width: 300
        height: 600
        
        // Rotate with roll
        transform: Rotation {
            origin.x: pitchLadder.width / 2
            origin.y: pitchLadder.height / 2
            angle: -root.roll
        }
        
        // Translate with pitch (e.g., 5 pixels per degree)
        y: (parent.height - height) / 2 + (root.pitch * 5)
        
        Repeater {
            model: 19 // -90 to +90 in 10 degree increments
            Item {
                width: 300
                height: 20
                y: pitchLadder.height / 2 - (index - 9) * 50 // 10 degrees * 5 px/deg = 50px
                
                property int deg: (index - 9) * 10
                
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    visible: deg !== 0
                    
                    Text { text: Math.abs(deg); color: root.hudColor; font.pixelSize: 12; width: 20; horizontalAlignment: Text.AlignRight }
                    
                    // Left line
                    Rectangle {
                        width: 80; height: 2; color: root.hudColor; opacity: 0.8
                        // Dashed if negative pitch (dive)
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 10 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 30 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 50 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 70 }
                    }
                    
                    // Gap
                    Item { width: 40; height: 2 }
                    
                    // Right line
                    Rectangle {
                        width: 80; height: 2; color: root.hudColor; opacity: 0.8
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 10 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 30 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 50 }
                        Rectangle { anchors.fill: parent; color: "black"; visible: deg < 0; width: 4; x: 70 }
                    }
                    
                    Text { text: Math.abs(deg); color: root.hudColor; font.pixelSize: 12; width: 20 }
                }
                
                // Horizon line
                Rectangle {
                    anchors.centerIn: parent
                    width: 250; height: 2; color: root.hudColor
                    visible: deg === 0
                }
            }
        }
    }

    // 2. FIXED CROSSHAIR
    Item {
        anchors.centerIn: parent
        width: 100; height: 100
        
        Rectangle { x: 30; y: 50; width: 15; height: 2; color: root.hudColor }
        Rectangle { x: 45; y: 50; width: 2; height: 15; color: root.hudColor }
        Rectangle { x: 55; y: 50; width: 15; height: 2; color: root.hudColor }
        Rectangle { x: 53; y: 50; width: 2; height: 15; color: root.hudColor }
        Rectangle { anchors.centerIn: parent; width: 4; height: 4; radius: 2; color: root.hudColor }
    }

    // 3. HEADING TAPE (Top)
    Item {
        anchors.top: parent.top
        anchors.topMargin: 70
        anchors.horizontalCenter: parent.horizontalCenter
        width: 300
        height: 40
        clip: true

        Rectangle { anchors.fill: parent; color: "#AA000000"; border.color: root.hudColor; border.width: 1 }
        
        Rectangle {
            anchors.centerIn: parent; width: 2; height: 40; color: root.hudColor
        }

        Item {
            width: 360 * 5 // 5 pixels per degree
            height: 40
            x: (300 / 2) - (root.yaw * 5)
            
            Repeater {
                model: 36
                Item {
                    x: index * 10 * 5
                    width: 2; height: 10
                    Rectangle { anchors.fill: parent; color: root.hudColor }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.bottom
                        anchors.topMargin: 2
                        text: index === 0 ? "N" : (index === 9 ? "E" : (index === 18 ? "S" : (index === 27 ? "W" : index * 10)))
                        color: root.hudColor
                        font.pixelSize: 12
                        font.bold: index % 9 === 0
                    }
                }
            }
        }
    }

    // 4. SPEED TAPE (Left)
    Item {
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 60
        height: 300

        Rectangle { anchors.fill: parent; color: "#44000000"; border.color: root.hudColor; border.width: 1 }
        
        Text {
            anchors.top: parent.top
            anchors.topMargin: -20
            anchors.horizontalCenter: parent.horizontalCenter
            text: "SPD(m/s)"
            color: root.hudColor
            font.pixelSize: 10
        }
        
        Text {
            anchors.centerIn: parent
            text: root.currentSpeedMs.toFixed(1)
            color: root.hudColor
            font.pixelSize: 18
            font.bold: true
        }
    }

    // 5. TOF PROXIMITY TAPE (Right)
    Item {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 60
        height: 300

        Rectangle { anchors.fill: parent; color: "#44000000"; border.color: root.hudColor; border.width: 1 }
        
        Text {
            anchors.top: parent.top
            anchors.topMargin: -20
            anchors.horizontalCenter: parent.horizontalCenter
            text: "DIST(mm)"
            color: root.hudColor
            font.pixelSize: 10
        }
        
        Text {
            anchors.centerIn: parent
            text: root.frontDistMm > 0 ? root.frontDistMm : "---"
            color: root.frontDistMm > 0 && root.frontDistMm < 300 ? "#FF1744" : root.hudColor
            font.pixelSize: 18
            font.bold: true
        }
    }
}
