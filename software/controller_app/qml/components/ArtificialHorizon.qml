import QtQuick 2.15

Item {
    id: root
    width: 250
    height: 250
    
    property real pitch: 0.0 // -90 to +90
    property real roll: 0.0  // -180 to +180
    
    clip: true
    
    Rectangle {
        id: sky
        width: root.width * 2
        height: root.height * 2
        color: "#4A90E2"
        x: -root.width / 2
        y: -root.height / 2 + (root.pitch * root.height / 90.0) // Pitch translation
        
        transform: Rotation {
            origin.x: sky.width / 2
            origin.y: sky.height / 2
            angle: -root.roll // Roll rotation
        }
        
        Rectangle {
            id: ground
            width: sky.width
            height: sky.height / 2
            y: sky.height / 2
            color: "#8B5A2B"
        }
        
        // Horizon line
        Rectangle {
            width: sky.width
            height: 2
            y: sky.height / 2
            color: "white"
        }
    }
    
    // Inverse Mask to hide corners (perfect circular clipping without OpacityMask)
    Rectangle {
        anchors.centerIn: parent
        width: parent.width + 1000 // Huge to cover all overflow
        height: parent.height + 1000
        radius: width / 2
        color: "transparent"
        border.color: "#121212" // Matches the main window background (#121212)
        border.width: 500
    }
    
    // Outer Bezel Frame
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: "#444444"
        border.width: 3
    }
    
    // Glass reflection overlay for 3D effect
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.20) }
            GradientStop { position: 0.35; color: Qt.rgba(1.0, 1.0, 1.0, 0.0) }
            GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.40) }
        }
    }
    
    // Crosshair reference (Center Dot)
    Rectangle {
        anchors.centerIn: parent
        width: 14
        height: 14
        radius: 7
        color: "transparent"
        border.color: "#00E676"
        border.width: 2
    }
    
    // Left Wing
    Rectangle {
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width * 0.15
        height: 3
        color: "#00E676"
    }
    
    // Right Wing
    Rectangle {
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width * 0.15
        height: 3
        color: "#00E676"
    }

}
