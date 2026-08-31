import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property bool axisXEnabled: true
    property bool axisYEnabled: true
    
    // Values from -1023 to 1023
    property int axisX: 0
    property int axisY: 0
    
    // Exponential response curve filtering: u_exp(v) = sgn(v) * |v|^1.6
    function applyCurve(v) {
        let norm = v / 1023.0;
        let sign = norm < 0 ? -1 : 1;
        let expVal = sign * Math.pow(Math.abs(norm), 1.6);
        return Math.round(expVal * 1023);
    }
    
    function setExternal(xNorm, yNorm) {
        // xNorm, yNorm are from -1.0 to 1.0
        stick.x = (base.width - stick.width) / 2 + (xNorm * (base.width / 2.2));
        stick.y = (base.height - stick.height) / 2 - (yNorm * (base.height / 2.2));
        
        root.axisX = applyCurve(xNorm * 1023);
        root.axisY = applyCurve(yNorm * 1023);
    }
    
    Rectangle {
        id: base
        anchors.fill: parent
        radius: width / 2
        color: "#2c2c2c"
        border.color: "#555"
        border.width: 2
        
        Rectangle {
            id: stick
            width: root.width / 2.5
            height: root.height / 2.5
            radius: width / 2
            
            // Fake 3D gradient effect using a solid color with an inner border overlay
            color: "#00E5FF"
            border.color: "#0088AA"
            border.width: 4
            
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.7
                height: parent.height * 0.7
                radius: width / 2
                color: "#18FFFF"
                opacity: 0.8
            }
            
            x: (base.width - width) / 2
            y: (base.height - height) / 2
            
            Behavior on x { SpringAnimation { spring: 3; damping: 0.2; mass: 0.5 } }
            Behavior on y { SpringAnimation { spring: 3; damping: 0.2; mass: 0.5 } }
        }
        
        MultiPointTouchArea {
            anchors.fill: parent
            touchPoints: [ TouchPoint { id: tp } ]
            
            onUpdated: {
                if (tp.pressed) {
                    let newX = axisXEnabled ? Math.max(0, Math.min(base.width, tp.x)) : base.width / 2;
                    let newY = axisYEnabled ? Math.max(0, Math.min(base.height, tp.y)) : base.height / 2;
                    
                    stick.x = newX - stick.width / 2;
                    stick.y = newY - stick.height / 2;
                    
                    // Normalize to -1023 to +1023
                    let rawX = ((newX / base.width) * 2 - 1) * 1023;
                    let rawY = -((newY / base.height) * 2 - 1) * 1023; // Invert Y
                    
                    root.axisX = applyCurve(rawX);
                    root.axisY = applyCurve(rawY);
                }
            }
            onReleased: {
                stick.x = (base.width - stick.width) / 2;
                stick.y = (base.height - stick.height) / 2;
                root.axisX = 0;
                root.axisY = 0;
            }
        }
    }
}
