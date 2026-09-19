import QtQuick 2.15

Item {
    id: root
    
    property real pitch: 0.0 // -90 to +90
    property real roll: 0.0  // -180 to +180
    property real yaw: 0.0   // 0 to 360
    property real altitude: 0.0 // Relative altitude in meters
    
    // Smooth the inputs slightly for fluid animations
    property real smoothedAltitude: altitude
    Behavior on smoothedAltitude { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
    
    clip: true
    
    // Smooth the inputs slightly for fluid animations
    property real smoothedPitch: pitch
    property real smoothedRoll: roll
    Behavior on smoothedPitch { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
    Behavior on smoothedRoll { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
    
    // 1. Compass Ribbon (Top)
    Item {
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(600, parent.width * 0.8)
        height: 45
        clip: true
        
        // Background strip
        Rectangle {
            anchors.fill: parent
            color: "#66000000"
            border.color: "#00FFCC"
            border.width: 1
            radius: 4
        }
        
        // Sliding Tape
        Row {
            // 4 pixels per degree. We render 3 full 360-degree cycles to easily handle wrap-around without visual popping.
            // Center the middle cycle (which starts at 360).
            x: (parent.width / 2) - (root.yaw * 4) - (360 * 4)
            
            Repeater {
                model: 108 // 3 cycles * 36 segments (every 10 degrees)
                Item {
                    width: 40 // 10 degrees * 4 pixels/degree
                    height: 45
                    
                    property int deg: (index % 36) * 10
                    property string lbl: deg === 0 ? "N" : deg === 90 ? "E" : deg === 180 ? "S" : deg === 270 ? "W" : ""
                    
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 2
                        height: lbl !== "" ? 12 : 6
                        color: "#00FFCC"
                    }
                    
                    Text {
                        anchors.top: parent.top
                        anchors.topMargin: 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: lbl !== "" ? lbl : deg
                        color: lbl !== "" ? "yellow" : "#00FFCC"
                        font.pixelSize: lbl !== "" ? 16 : 11
                        font.bold: lbl !== ""
                    }
                }
            }
        }
        
        // Center fixed tick mark (Current Heading)
        Rectangle {
            anchors.centerIn: parent
            width: 3
            height: parent.height
            color: "yellow"
            z: 2
        }
    }
    
    // 2. Trajectory / Drive Path Overlay (Perspective Grid)
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            
            var centerX = width / 2;
            var bottomY = height;
            var horizonY = height * 0.45; // Horizon line height
            
            // Draw main path lines (simulating rover width projected forward)
            ctx.beginPath();
            ctx.moveTo(centerX - 200, bottomY);
            ctx.lineTo(centerX - 50, horizonY);
            
            ctx.moveTo(centerX + 200, bottomY);
            ctx.lineTo(centerX + 50, horizonY);
            
            ctx.strokeStyle = "rgba(0, 255, 204, 0.5)";
            ctx.lineWidth = 3;
            ctx.stroke();
            
            // Draw horizontal distance markers
            for (var i = 1; i <= 5; i++) {
                var factor = i / 6.0; // Distance scaling
                var y = bottomY - ((bottomY - horizonY) * factor);
                var leftX = centerX - 200 + ((200 - 50) * factor);
                var rightX = centerX + 200 - ((200 - 50) * factor);
                
                ctx.beginPath();
                ctx.moveTo(leftX, y);
                ctx.lineTo(rightX, y);
                ctx.strokeStyle = "rgba(0, 255, 204, 0.3)";
                ctx.lineWidth = 1;
                ctx.stroke();
            }
        }
        // Redraw if window size changes
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }
    
    // 3. Inclinometer (Tilt Gauge)
    Item {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        anchors.horizontalCenter: parent.horizontalCenter
        width: 140
        height: 140
        
        // Outer ring
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "#66000000"
            border.color: "#00FFCC"
            border.width: 2
            
            // Inner safe-zone ring (e.g., 20 degrees tilt limit)
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.44
                height: parent.height * 0.44
                radius: width / 2
                color: "transparent"
                border.color: "yellow"
                border.width: 1
                opacity: 0.5
            }
            
            // Crosshairs
            Rectangle { anchors.centerIn: parent; width: parent.width; height: 1; color: "#00FFCC"; opacity: 0.5 }
            Rectangle { anchors.centerIn: parent; width: 1; height: parent.height; color: "#00FFCC"; opacity: 0.5 }
        }
        
        // The "Bubble" indicating tilt
        // Center is (62, 62) for a 16x16 bubble in a 140x140 circle.
        // Max tilt displayed = 45 degrees. 45 deg = 70 pixels from center.
        Rectangle {
            width: 16
            height: 16
            radius: 8
            color: "yellow"
            
            property real cx: 70 - 8 + (root.smoothedRoll / 45.0) * 70
            property real cy: 70 - 8 - (root.smoothedPitch / 45.0) * 70
            
            // Clamp it inside the circle visually
            property real dist: Math.sqrt(Math.pow((root.smoothedRoll / 45.0) * 70, 2) + Math.pow((root.smoothedPitch / 45.0) * 70, 2))
            
            x: dist > 62 ? (70 - 8 + ((root.smoothedRoll / 45.0) * 70) * (62 / dist)) : cx
            y: dist > 62 ? (70 - 8 - ((root.smoothedPitch / 45.0) * 70) * (62 / dist)) : cy
        }
        
        // Digital readouts below the inclinometer
        Row {
            anchors.top: parent.bottom
            anchors.topMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20
            
            Text { text: "P: " + Math.round(root.pitch) + "°"; color: "white"; font.pixelSize: 14; font.bold: true; font.family: "Monospace" }
            Text { text: "R: " + Math.round(root.roll) + "°"; color: "white"; font.pixelSize: 14; font.bold: true; font.family: "Monospace" }
        }
    }
    
    // 4. Altimeter Tape (Right side)
    Item {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 60
        height: Math.min(400, parent.height * 0.8)
        clip: true
        
        // Background strip
        Rectangle {
            anchors.fill: parent
            color: "#66000000"
            border.color: "#00FFCC"
            border.width: 1
            radius: 4
        }
        
        // Sliding Tape
        Column {
            // Let's do 1 meter = 10 pixels. Center is parent.height/2.
            // Center is mapped to root.smoothedAltitude
            y: (parent.height / 2) + (root.smoothedAltitude * 10) - (100 * 10)
            
            Repeater {
                model: 201 // -100 to +100
                Item {
                    width: 60
                    height: 10 // 1 pixel = 0.1m, so 10 pixels = 1 meter
                    
                    property int altM: 100 - index
                    property bool majorTick: Math.abs(altM) % 5 === 0
                    
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: majorTick ? 12 : 6
                        height: 2
                        color: "#00FFCC"
                    }
                    
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        text: majorTick ? altM : ""
                        color: altM === 0 ? "yellow" : "#00FFCC"
                        font.pixelSize: 12
                        font.bold: majorTick
                    }
                }
            }
        }
        
        // Center fixed tick mark (Current Altitude)
        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 3
            color: "yellow"
            z: 2
        }
        
        // Digital readout overlay
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: -35
            width: 50
            height: 24
            color: "black"
            border.color: "yellow"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: root.smoothedAltitude.toFixed(1) + "m"
                color: "yellow"
                font.pixelSize: 12
                font.bold: true
                font.family: "Monospace"
            }
        }
    }
}
