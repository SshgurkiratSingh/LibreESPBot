import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property bool axisXEnabled: true
    property bool axisYEnabled: true
    
    // Exponential response curve filtering: u_exp(v) = sgn(v) * |v|^1.6
    function applyCurve(v) {
        let norm = v / 1023.0;
        let sign = norm < 0 ? -1 : 1;
        let expVal = sign * Math.pow(Math.abs(norm), 1.6);
        return Math.round(expVal * 1023);
    }
    
    // Reverse exponential curve to find physical stick position from backend value
    function reverseCurve(v) {
        let norm = v / 1023.0;
        let sign = norm < 0 ? -1 : 1;
        let linearNorm = sign * Math.pow(Math.abs(norm), 1.0 / 1.6);
        return linearNorm;
    }
    
    Connections {
        target: (typeof commandEmitter !== "undefined" && commandEmitter) ? commandEmitter : null
        function onCurrentThrottleChanged() {
            if (!tp.pressed) {
                let v = commandEmitter.currentThrottle;
                let limit = (typeof appSettings !== "undefined") ? (appSettings.maxThrottleLimit / 100.0) : 1.0;
                if (limit > 0) v = v / limit; // Reverse limit scaling for UI
                let linearNorm = reverseCurve(v);
                stick.y = (base.height - stick.height) / 2 - (linearNorm * (base.height / 2.2));
            }
        }
        function onCurrentSteeringChanged() {
            if (!tp.pressed) {
                let v = commandEmitter.currentSteering;
                let sens = (typeof appSettings !== "undefined") ? appSettings.steeringSensitivity : 1.0;
                if (sens > 0) v = v / sens; // Reverse sens scaling for UI
                let linearNorm = reverseCurve(v);
                stick.x = (base.width - stick.width) / 2 + (linearNorm * (base.width / 2.2));
            }
        }
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
                    
                    if (typeof commandEmitter !== "undefined") {
                        let rawX = ((newX / base.width) * 2 - 1) * 1023;
                        let rawY = -((newY / base.height) * 2 - 1) * 1023;
                        
                        let axisX = applyCurve(rawX);
                        let axisY = applyCurve(rawY);
                        
                        let sens = (typeof appSettings !== "undefined") ? appSettings.steeringSensitivity : 1.0;
                        let limit = (typeof appSettings !== "undefined") ? (appSettings.maxThrottleLimit / 100.0) : 1.0;
                        
                        commandEmitter.updateSteering(axisX * sens);
                        commandEmitter.updateThrottle(axisY * limit);
                    }
                }
            }
            onReleased: {
                stick.x = (base.width - stick.width) / 2;
                stick.y = (base.height - stick.height) / 2;
                if (typeof commandEmitter !== "undefined") {
                    commandEmitter.updateSteering(0);
                    commandEmitter.updateThrottle(0);
                }
            }
        }
    }
}
