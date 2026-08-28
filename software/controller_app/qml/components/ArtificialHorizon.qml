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
        border.color: "black" // Matches the main window background
        border.width: 500
    }
    
    // Crosshair reference
    Rectangle {
        anchors.centerIn: parent
        width: 10
        height: 10
        radius: 5
        color: "yellow"
    }
    Rectangle {
        anchors.centerIn: parent
        width: 150
        height: 2
        color: "yellow"
    }
    
    // Border
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "white"
        border.width: 2
        radius: width / 2
    }
}
