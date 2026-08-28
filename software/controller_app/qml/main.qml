import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import "components"
import "theme"

Window {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    visibility: Qt.platform.os === "android" ? Window.FullScreen : Window.Windowed
    title: qsTr("ESP32 Rover Ground Station")
    color: "#121212"
    
    // Globally accessible property
    property bool isMobilePortrait: width < height
    
    Material.theme: Material.Dark
    Material.accent: Material.Green

    // Keyboard Input State
    property real keyThrottle: 0.0
    property real keySteering: 0.0
    
    property bool enable3dKinematics: false

    onKeyThrottleChanged: if (typeof mainJoystick !== "undefined" && mainJoystick !== null) mainJoystick.setExternal(keySteering, keyThrottle)
    onKeySteeringChanged: if (typeof mainJoystick !== "undefined" && mainJoystick !== null) mainJoystick.setExternal(keySteering, keyThrottle)

    property var radarDataMap: ({})
    property var currentRadarPoints: []

    Connections {
        target: typeof telemetryClient !== "undefined" ? telemetryClient : null
        function onTelemetryUpdated() {
            if (!telemetryClient) return;
            
            var angle = telemetryClient.servoAngleDeg; // -90 to +90
            var dist1 = telemetryClient.tof1DistMm;
            var dist2 = telemetryClient.tof2DistMm;
            
            // Filter invalid readings (0 or > 2000)
            if (dist1 > 0 && dist1 < 2000) {
                var rad1 = angle * Math.PI / 180.0;
                // Assuming 0 degrees is straight ahead (Y-axis)
                radarDataMap["s1_" + angle] = { x: Math.sin(rad1) * dist1, y: Math.cos(rad1) * dist1, ts: Date.now() };
            }
            if (dist2 > 0 && dist2 < 2000) {
                var rad2 = (angle + 180) * Math.PI / 180.0;
                radarDataMap["s2_" + angle] = { x: Math.sin(rad2) * dist2, y: Math.cos(rad2) * dist2, ts: Date.now() };
            }
            
            // Clean up old points (fade out after 5 seconds) to avoid memory leaks
            var now = Date.now();
            var newPoints = [];
            var tempMap = radarDataMap; // Work with reference
            
            for (var key in tempMap) {
                if (!tempMap.hasOwnProperty(key)) continue;
                
                var pt = tempMap[key];
                if (!pt || pt === undefined) continue; // Safety check
                
                if (now - pt.ts > 5000) {
                    delete tempMap[key];
                } else {
                    newPoints.push(pt);
                }
            }
            
            radarDataMap = tempMap; // Reassign to maintain state safely
            currentRadarPoints = newPoints;
        }
    }

    Item {
        id: rootItem
        anchors.fill: parent
        focus: true // Necessary to capture keyboard events

        Keys.onPressed: (event) => {
            if (event.isAutoRepeat) return;
            if (event.key === Qt.Key_W || event.key === Qt.Key_Up) keyThrottle = 1.0;
            else if (event.key === Qt.Key_S || event.key === Qt.Key_Down) keyThrottle = -1.0;
            else if (event.key === Qt.Key_A || event.key === Qt.Key_Left) keySteering = -1.0;
            else if (event.key === Qt.Key_D || event.key === Qt.Key_Right) keySteering = 1.0;
        }

        Keys.onReleased: (event) => {
            if (event.isAutoRepeat) return;
            if (event.key === Qt.Key_W || event.key === Qt.Key_Up || event.key === Qt.Key_S || event.key === Qt.Key_Down) keyThrottle = 0;
            else if (event.key === Qt.Key_A || event.key === Qt.Key_Left || event.key === Qt.Key_D || event.key === Qt.Key_Right) keySteering = 0;
        }

        GridLayout {
            anchors.fill: parent
            anchors.margins: 10
            rowSpacing: 15
            columnSpacing: 15
            
            columns: mainWindow.isMobilePortrait ? 1 : 3

            // Left Sidebar: Radar & Hardware Inspector
            Rectangle {
                Layout.fillHeight: !mainWindow.isMobilePortrait
                Layout.preferredHeight: mainWindow.isMobilePortrait ? 400 : -1
                Layout.preferredWidth: mainWindow.isMobilePortrait ? parent.width : mainWindow.width * 0.25
                color: "#1e1e1e"
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20

                    Text {
                        text: "RADAR SWEEP"
                        color: "#00E5FF" // Cyan
                        font.pixelSize: 18
                        font.bold: true
                    }

                    PolarRadarCanvas {
                        id: radarCanvas
                        Layout.fillWidth: true
                        Layout.preferredHeight: width
                        points: currentRadarPoints
                    }
                    
                    Item { Layout.fillHeight: true } // spacer

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        
                        HardwareInspector {
                            id: hwInspector
                            width: parent.width
                        }
                    }
                }
            }

            // Center Viewport: Video & Horizon
            Rectangle {
                Layout.fillHeight: true
                Layout.minimumHeight: mainWindow.isMobilePortrait ? 400 : 0
                Layout.fillWidth: true
                color: "black"
                radius: 10
                
                // Placeholder for VideoViewport fills the background
                VideoViewport {
                    anchors.fill: parent
                    visible: !enable3dKinematics
                }
                
                // 3D Kinematics Placeholder (Visible when camera is disabled)
                Rectangle {
                    anchors.fill: parent
                    color: "#111"
                    visible: enable3dKinematics
                    
                    KinematicsView3D {
                        anchors.fill: parent
                        pitch: typeof telemetryClient !== "undefined" ? telemetryClient.pitch : 0.0
                        roll: typeof telemetryClient !== "undefined" ? telemetryClient.roll : 0.0
                        yaw: typeof telemetryClient !== "undefined" ? telemetryClient.yaw : 0.0
                    }
                }

                // Premium HUD Overlay (Top Pill)
                Rectangle {
                    anchors.top: parent.top
                    anchors.topMargin: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 140
                    height: 36
                    radius: 18
                    color: Qt.rgba(0.0, 0.0, 0.0, 0.6)
                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.1)
                    border.width: 1
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        spacing: 10
                        
                        Text {
                            text: "HDG"
                            color: "#888888"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1
                            Layout.alignment: Qt.AlignVCenter
                        }
                        
                        Text {
                            property real hdg: typeof telemetryClient !== "undefined" ? telemetryClient.headingCompassDeg : 0.0
                            // Normalize to 0-360 for compass reading
                            property real compassHdg: (hdg % 360 + 360) % 360
                            
                            text: Math.round(compassHdg) + "°"
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                            font.family: "Monospace"
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                // Small Artificial Horizon overlay in the top-right corner
                ArtificialHorizon {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 20
                    width: Math.min(150, parent.width * 0.3)
                    height: width
                    pitch: typeof telemetryClient !== "undefined" ? telemetryClient.pitch : 0.0
                    roll: typeof telemetryClient !== "undefined" ? telemetryClient.roll : 0.0
                }
                
                // Settings button overlay in the top-left corner
                Button {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 20
                    text: "⚙ Settings"
                    onClicked: settingsDrawer.open()
                }
            }

            // Right Sidebar: Controls
            Rectangle {
                Layout.fillHeight: !mainWindow.isMobilePortrait
                Layout.preferredHeight: mainWindow.isMobilePortrait ? 450 : -1
                Layout.preferredWidth: mainWindow.isMobilePortrait ? parent.width : mainWindow.width * 0.25
                color: "#1e1e1e"
                radius: 10

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 15
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                    Text {
                        text: "ACTUATION"
                        color: "#00E676" // Green
                        font.pixelSize: 18
                        font.bold: true
                    }

                    // Single Unified Joystick
                    VirtualJoystick {
                        id: mainJoystick
                        Layout.preferredWidth: parent.width * 0.8
                        Layout.preferredHeight: width
                        Layout.alignment: Qt.AlignHCenter
                        axisXEnabled: true
                        axisYEnabled: true
                        
                        onAxisYChanged: if (typeof commandEmitter !== "undefined") commandEmitter.updateThrottle(axisY)
                        onAxisXChanged: if (typeof commandEmitter !== "undefined") commandEmitter.updateSteering(axisX)
                    }

                    Item { Layout.fillHeight: true }
                    
                    // Automations Drawer Toggles
                    Switch { 
                        text: "Auto Emergency Brake (AEB)"
                        onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setAutoBrake(checked)
                    }
                    Switch { 
                        text: "APF Collision Avoidance" 
                        onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setApfAvoidance(checked)
                    }
                    Switch { 
                        text: "Radar Sweep" 
                        onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setRadarSweep(checked)
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Speed Mode:"; color: "white" }
                        ComboBox {
                            id: speedModeCombo
                            Layout.fillWidth: true
                            model: ["Precision (30%)", "Normal (70%)", "Sport (100%)"]
                            currentIndex: 1 // Default to Normal
                            onCurrentIndexChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setSpeedMode(currentIndex)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Headlights:"; color: "white" }
                        ComboBox {
                            id: headlightCombo
                            Layout.fillWidth: true
                            model: ["Off", "On (White)", "Police Strobe", "Custom...", "Rainbow", "Cylon Scanner"]
                            onCurrentIndexChanged: {
                                if (typeof commandEmitter !== "undefined") {
                                    commandEmitter.setHeadlightMode(currentIndex)
                                }
                                if (currentIndex === 3) {
                                    customColorDialog.open()
                                }
                            }
                        }
                    }
                    }
                } // End ScrollView
            } // End Rectangle
        }
    }

    ColorDialog {
        id: customColorDialog
        title: "Choose Custom LED Color"
        onAccepted: {
            if (typeof commandEmitter !== "undefined") {
                commandEmitter.setCustomLedColor(
                    Math.round(selectedColor.r * 255),
                    Math.round(selectedColor.g * 255),
                    Math.round(selectedColor.b * 255)
                )
            }
        }
        onRejected: {
            if (headlightCombo.currentIndex === 3) {
                headlightCombo.currentIndex = 0
            }
        }
    }

    // Settings Drawer for parameters configuration
    Drawer {
        id: settingsDrawer
        width: parent.width // Full screen responsive width
        height: parent.height // Full screen responsive height
        edge: Qt.LeftEdge
        
        background: Rectangle {
            color: "#252525"
            border.color: "#444"
        }

        ScrollView {
            anchors.fill: parent
            anchors.margins: 40 // Larger margins for full screen visibility
            contentWidth: availableWidth // Fix horizontal scrolling bug
            clip: true
            
            ColumnLayout {
                width: parent.width
                spacing: 20

                Text {
                    text: "SYSTEM SETTINGS"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555"
                }

                Text { text: "Display Configuration"; color: "white"; font.bold: true }
                
                Switch {
                    Layout.fillWidth: true
                    text: "3D Kinematics View (Disable Camera)"
                    checked: enable3dKinematics
                    onCheckedChanged: enable3dKinematics = checked
                }
                
                Switch {
                    Layout.fillWidth: true
                    text: "HUD Debug Overlays"
                    checked: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555"
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                }

                Text { text: "Rover Chassis & Control"; color: "white"; font.bold: true }

                ComboBox {
                    Layout.fillWidth: true
                    model: ["Tank Drive (Differential)", "Ackermann Steering", "Mecanum (Omni)"]
                }
                
                Text { text: "Max Throttle Limit"; color: "gray"; font.pixelSize: 12 }
                Slider {
                    Layout.fillWidth: true
                    from: 0; to: 100; value: 100
                }
                
                Text { text: "Steering Sensitivity / Expo"; color: "gray"; font.pixelSize: 12 }
                Slider {
                    Layout.fillWidth: true
                    from: 0.1; to: 2.0; value: 1.0
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555"
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                }

                Text { text: "Hardware & Telemetry"; color: "white"; font.bold: true }

                ComboBox {
                    Layout.fillWidth: true
                    model: ["V1 (ESP8266 + L298N)", "V2 (ESP32 + TB6612)", "V3 (CAN / ODrive)"]
                    currentIndex: 1 // Default to V2
                }
                
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Telemetry: 50 Hz (Fast)", "Telemetry: 20 Hz (Normal)", "Telemetry: 5 Hz (Low Bandwidth)"]
                }
                
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Motor PWM: 1 kHz", "Motor PWM: 20 kHz (Silent)", "Motor PWM: 500 Hz"]
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555"
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                }

                Text { text: "Safety & Fusion Overrides"; color: "white"; font.bold: true }
                
                Text { text: "Low Battery Warning (Volts)"; color: "gray"; font.pixelSize: 12 }
                Slider {
                    Layout.fillWidth: true
                    from: 9.0; to: 12.6; value: 11.1
                }
                
                Text { text: "Collision Stop Distance (mm)"; color: "gray"; font.pixelSize: 12 }
                Slider {
                    Layout.fillWidth: true
                    from: 50; to: 500; value: 150
                }
                
                ComboBox {
                    Layout.fillWidth: true
                    model: ["Failsafe Timeout: 500ms", "Failsafe Timeout: 1s", "Failsafe Timeout: 2s"]
                }
                
                Text { text: "Mahony AHRS Kp Gain"; color: "gray"; font.pixelSize: 12 }
                TextField {
                    Layout.fillWidth: true
                    text: "10.0"
                }
                
                Text { text: "Mahony AHRS Ki Gain"; color: "gray"; font.pixelSize: 12 }
                TextField {
                    Layout.fillWidth: true
                    text: "0.0"
                }
            }
        }
    }
}
