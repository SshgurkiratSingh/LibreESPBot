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
    title: qsTr("LibreESPBot")
    color: "#121212"
    
    // Globally accessible property
    property bool isMobilePortrait: width < height
    
    Material.theme: Material.Dark
    Material.accent: Material.Green

    // Keyboard Input State
    property real keyThrottle: 0.0
    property real keySteering: 0.0
    property int previousSpeedMode: 1
    
    property bool enable3dKinematics: false
    property bool reverseTofSensors: false
    property real voltageMultiplier: (typeof appSettings !== "undefined") ? appSettings.voltageScaleMultiplier : 1.0

    property double lastTelemetryTime: 0
    property bool isConnected: false

    Timer {
        id: connectionWatchdog
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            mainWindow.isConnected = (Date.now() - mainWindow.lastTelemetryTime) < 1500;
        }
    }

    onKeyThrottleChanged: if (typeof mainJoystick !== "undefined" && mainJoystick !== null) mainJoystick.setExternal(keySteering, keyThrottle)
    onKeySteeringChanged: if (typeof mainJoystick !== "undefined" && mainJoystick !== null) mainJoystick.setExternal(keySteering, keyThrottle)

    onActiveChanged: {
        if (!active) {
            // Safety: release all keys if window loses focus
            keyThrottle = 0;
            keySteering = 0;
        } else {
            // Re-gain focus to allow keyboard events immediately
            if (typeof rootItem !== "undefined") rootItem.forceActiveFocus();
        }
    }

    property var radarDataMap: ({})
    property var currentRadarPoints: []

    Connections {
        target: typeof telemetryClient !== "undefined" ? telemetryClient : null
        function onTelemetryUpdated() {
            if (!telemetryClient) return;
            
            var angle = telemetryClient.servoAngleDeg; // -90 to +90
            var dist1 = telemetryClient.tof1DistMm;
            var dist2 = telemetryClient.tof2DistMm;
            
            // Apply Sensor Base Angle Offset
            var angleOffset = (typeof appSettings !== "undefined") ? appSettings.sensorBaseAngleDeg : 0;
            var finalAngle = angle + angleOffset;
            
            // Invert Left/Right ToF Sensors
            var invert = (typeof appSettings !== "undefined") ? appSettings.invertTof : false;
            if (invert) {
                var temp = dist1;
                dist1 = dist2;
                dist2 = temp;
            }

            // Filter invalid readings (0 or > 2000)
            if (dist1 > 0 && dist1 < 2000) {
                var rad1 = finalAngle * Math.PI / 180.0;
                // Assuming 0 degrees is straight ahead (Y-axis)
                radarDataMap["s1_" + finalAngle] = { x: Math.sin(rad1) * dist1, y: Math.cos(rad1) * dist1, ts: Date.now() };
            }
            if (dist2 > 0 && dist2 < 2000) {
                var rad2 = (finalAngle + 180) * Math.PI / 180.0;
                radarDataMap["s2_" + finalAngle] = { x: Math.sin(rad2) * dist2, y: Math.cos(rad2) * dist2, ts: Date.now() };
            }
            
            // Clean up old points (fade out based on settings) to avoid memory leaks
            var now = Date.now();
            var newPoints = [];
            var tempMap = radarDataMap; // Work with reference
            var pointLifetime = (typeof appSettings !== "undefined") ? appSettings.radarPointLifetimeMs : 5000;
            
            for (var key in tempMap) {
                if (!tempMap.hasOwnProperty(key)) continue;
                
                var pt = tempMap[key];
                if (!pt || pt === undefined) continue; // Safety check
                
                if (now - pt.ts > pointLifetime) {
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
            else if (event.key === Qt.Key_Shift) {
                previousSpeedMode = speedModeCombo.currentIndex;
                speedModeCombo.currentIndex = 3; // Sport
            }
            else if (event.key === Qt.Key_B) aebSwitch.checked = !aebSwitch.checked;
            else if (event.key === Qt.Key_V) apfSwitch.checked = !apfSwitch.checked;
            else if (event.key === Qt.Key_R) radarSwitch.checked = !radarSwitch.checked;
            else if (event.key === Qt.Key_O) alertSwitch.checked = !alertSwitch.checked;
            else if (event.key === Qt.Key_N) noLagSwitch.checked = !noLagSwitch.checked;
            else if (event.key === Qt.Key_F) reverseTofSwitch.checked = !reverseTofSwitch.checked;
            else if (event.key === Qt.Key_K) enable3dKinematics = !enable3dKinematics;
            else if (event.key === Qt.Key_1) speedModeCombo.currentIndex = 0;
            else if (event.key === Qt.Key_2) speedModeCombo.currentIndex = 1;
            else if (event.key === Qt.Key_3) speedModeCombo.currentIndex = 2;
            else if (event.key === Qt.Key_4) speedModeCombo.currentIndex = 3;
            else if (event.key === Qt.Key_H || event.key === Qt.Key_L) {
                headlightCombo.currentIndex = (headlightCombo.currentIndex + 1) % headlightCombo.model.length;
            }
        }

        Keys.onReleased: (event) => {
            if (event.isAutoRepeat) return;
            if (event.key === Qt.Key_W || event.key === Qt.Key_Up || event.key === Qt.Key_S || event.key === Qt.Key_Down) keyThrottle = 0;
            else if (event.key === Qt.Key_A || event.key === Qt.Key_Left || event.key === Qt.Key_D || event.key === Qt.Key_Right) keySteering = 0;
            else if (event.key === Qt.Key_Shift) {
                speedModeCombo.currentIndex = previousSpeedMode;
            }
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
                    visible: (typeof appSettings !== "undefined") ? !appSettings.display3dKinematics : true
                }
                
                // 3D Kinematics Placeholder (Visible when camera is disabled)
                Rectangle {
                    anchors.fill: parent
                    color: "#111"
                    visible: (typeof appSettings !== "undefined") ? appSettings.display3dKinematics : false
                    
                    KinematicsView3D {
                        anchors.fill: parent
                        pitch: typeof telemetryClient !== "undefined" ? telemetryClient.pitch : 0.0
                        roll: typeof telemetryClient !== "undefined" ? telemetryClient.roll : 0.0
                        yaw: typeof telemetryClient !== "undefined" ? telemetryClient.headingCompassDeg : 0.0
                    }
                }

                // Fullscreen Aviation HUD Overlay
                AviationHud {
                    anchors.fill: parent
                    visible: (typeof appSettings !== "undefined") ? appSettings.displayHudDebug : true
                    pitch: typeof telemetryClient !== "undefined" ? (telemetryClient.pitch - (typeof appSettings !== "undefined" ? appSettings.pitchOffset : 0)) : 0.0
                    roll: typeof telemetryClient !== "undefined" ? (telemetryClient.roll - (typeof appSettings !== "undefined" ? appSettings.rollOffset : 0)) : 0.0
                    yaw: typeof telemetryClient !== "undefined" ? telemetryClient.headingCompassDeg : 0.0
                }

                // Obstacle Alert Overlay (Clean Pill at Bottom)
                Rectangle {
                    id: obstacleAlertBanner
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 40
                    width: mainWindow.isMobilePortrait ? parent.width * 0.8 : 300
                    height: 44
                    color: "#CCFF0000" // Semi-transparent red
                    radius: 22
                    
                    // Logic for visibility:
                    property int tof1: typeof telemetryClient !== "undefined" ? telemetryClient.tof1DistMm : 8191
                    property int tof2: typeof telemetryClient !== "undefined" ? telemetryClient.tof2DistMm : 8191
                    
                    property bool hasObstacle: (tof1 > 30 && tof1 < 100) || (tof2 > 30 && tof2 < 100)
                    
                    property int closestDist: {
                        let t1 = (tof1 > 30 && tof1 < 8000) ? tof1 : 9999;
                        let t2 = (tof2 > 30 && tof2 < 8000) ? tof2 : 9999;
                        return Math.min(t1, t2);
                    }
                    
                    // Only show alert if moving forward or backward to prevent annoyance while stationary
                    property bool isMoving: keyThrottle !== 0 || (typeof mainJoystick !== "undefined" && Math.abs(mainJoystick.axisY) > 100)
                    
                    visible: alertSwitch.checked && hasObstacle && isMoving
                    
                    Text {
                        anchors.centerIn: parent
                        text: "OBSTACLE PROXIMITY ALERT (" + obstacleAlertBanner.closestDist + " mm)"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                    }
                    
                    Timer {
                        id: alertBlinker
                        interval: 200
                        repeat: true
                        running: obstacleAlertBanner.visible
                        onTriggered: obstacleAlertBanner.opacity = obstacleAlertBanner.opacity === 1.0 ? 0.3 : 1.0
                    }
                    onVisibleChanged: if (!visible) opacity = 1.0
                }

                // Old ArtificialHorizon removed in favor of AviationHud
                Connections {
                    target: typeof telemetryClient !== "undefined" ? telemetryClient : null
                    function onTelemetryUpdated() {
                        mainWindow.lastTelemetryTime = Date.now();
                        if (typeof telemetryClient !== "undefined" && telemetryClient !== null) {
                            // ... update any additional main UI state if needed
                        }
                    }
                }
                
                // Top-left overlay buttons
                Row {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 20
                    spacing: 10
                    
                    Button {
                        text: "⚙ Settings"
                        onClicked: settingsDrawer.open()
                    }
                    
                    Button {
                        text: "📝 Scripting"
                        onClicked: scriptPanelOverlay.visible = !scriptPanelOverlay.visible
                    }

                    Button {
                        text: "🛠 Execute Tool"
                        onClicked: executeToolPanelOverlay.visible = !executeToolPanelOverlay.visible
                    }
                }
            }

            // Right Sidebar: Controls
            Rectangle {
                Layout.fillHeight: !mainWindow.isMobilePortrait
                Layout.preferredHeight: mainWindow.isMobilePortrait ? 450 : -1
                Layout.preferredWidth: mainWindow.isMobilePortrait ? parent.width : mainWindow.width * 0.25
                color: "#1e1e1e"
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Text {
                        text: "ACTUATION"
                        color: "#00E676" // Green
                        font.pixelSize: 18
                        font.bold: true
                    }

                    // Single Unified Joystick (Fixed at top, outside ScrollView)
                    VirtualJoystick {
                        id: mainJoystick
                        Layout.preferredWidth: parent.width * 0.8
                        Layout.preferredHeight: width
                        Layout.alignment: Qt.AlignHCenter
                        axisXEnabled: true
                        axisYEnabled: true
                        
                        onAxisYChanged: {
                            if (typeof commandEmitter !== "undefined") {
                                var limit = (typeof appSettings !== "undefined") ? (appSettings.maxThrottleLimit / 100.0) : 1.0;
                                commandEmitter.updateThrottle(axisY * limit);
                            }
                        }
                        onAxisXChanged: {
                            if (typeof commandEmitter !== "undefined") {
                                var sens = (typeof appSettings !== "undefined") ? appSettings.steeringSensitivity : 1.0;
                                commandEmitter.updateSteering(axisX * sens);
                            }
                        }
                    }

                    // Scrollable settings area below the joystick
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth

                        ColumnLayout {
                            width: parent.width
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Speed Mode [1-4]:"; color: "white" }
                                ComboBox {
                                    id: speedModeCombo
                                    Layout.fillWidth: true
                                    model: ["Crawl (15%)", "Precision (30%)", "Normal (70%)", "Sport (100%)"]
                                    currentIndex: 2 // Default to Normal (index 2 now)
                                    onCurrentIndexChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setSpeedMode(currentIndex)
                                }
                            }
                            
                            Switch { 
                                id: reverseTofSwitch
                                text: "Reverse ToF (Front/Back) [F]" 
                                checked: reverseTofSensors
                                onCheckedChanged: reverseTofSensors = checked
                            }

                            
                            // Automations Drawer Toggles
                            Switch { 
                                id: noLagSwitch
                                text: "NoLag Hard Realtime [N]"
                                onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setNoLagMode(checked)
                            }
                            Switch { 
                                id: aebSwitch
                                text: "Auto Emergency Brake [B]"
                                onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setAutoBrake(checked)
                            }
                            Switch { 
                                id: apfSwitch
                                text: "APF Collision Avoidance [V]" 
                                onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setApfAvoidance(checked)
                            }
                            Switch { 
                                id: radarSwitch
                                text: "Radar Sweep [R]" 
                                onCheckedChanged: if (typeof commandEmitter !== "undefined") commandEmitter.setRadarSweep(checked)
                            }
                            Switch { 
                                id: alertSwitch
                                text: "Show Obstacle Alerts [O]" 
                                checked: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Headlights [H/L]:"; color: "white" }
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
                } // End ColumnLayout
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
        width: Math.min(parent.width * 0.9, 800) // Better responsive width for tabs
        height: parent.height
        edge: Qt.LeftEdge
        
        background: Rectangle {
            color: "#252525"
            border.color: "#444"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            Text {
                text: "SYSTEM SETTINGS"
                color: "white"
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            TabBar {
                id: settingsTabBar
                Layout.fillWidth: true
                background: Rectangle { color: "#333" }
                
                TabButton { text: "Network & Display" }
                TabButton { text: "Drive & Radar" }
                TabButton { text: "Hardware & Safety" }
                TabButton { text: "HUD Profile" }
            }

            SwipeView {
                id: settingsSwipeView
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: settingsTabBar.currentIndex
                onCurrentIndexChanged: settingsTabBar.currentIndex = currentIndex
                clip: true

                // --- TAB 1: Network & Display ---
                ScrollView {
                    contentWidth: availableWidth
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Text { text: "Network Connection"; color: "white"; font.bold: true; Layout.topMargin: 10 }
                        Text { text: "Rover IP Override"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "e.g., 192.168.4.1"
                            onEditingFinished: {
                                if (text.length > 0 && typeof discoveryWorker !== "undefined" && typeof commandEmitter !== "undefined") {
                                    discoveryWorker.setManualIp(text);
                                    commandEmitter.setTargetAddress(text, 8888);
                                }
                            }
                        }
                        
                        Text { text: "Camera IP Override"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            placeholderText: "e.g., 192.168.4.2"
                            onEditingFinished: {
                                if (text.length > 0 && typeof discoveryWorker !== "undefined") {
                                    discoveryWorker.setManualCameraIp(text);
                                }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#555" }
                        Text { text: "Display Configuration"; color: "white"; font.bold: true }
                        
                        Switch {
                            Layout.fillWidth: true
                            text: "3D Kinematics View (Disable Camera)"
                            checked: (typeof appSettings !== "undefined") ? appSettings.display3dKinematics : false
                            onCheckedChanged: if (typeof appSettings !== "undefined") appSettings.display3dKinematics = checked
                        }
                        
                        // HUD switch moved to Tab 4
                        
                        Item { Layout.fillHeight: true } // spacer
                    }
                }

                // --- TAB 2: Drive & Radar ---
                ScrollView {
                    contentWidth: availableWidth
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Text { text: "Rover Chassis & Control"; color: "white"; font.bold: true; Layout.topMargin: 10 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Tank Drive (Differential)", "Ackermann Steering", "Mecanum (Omni)"]
                        }
                        
                        Text { text: "Max Throttle Limit (%): " + throttleSlider.value.toFixed(0); color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: throttleSlider
                            Layout.fillWidth: true
                            from: 0; to: 100
                            value: (typeof appSettings !== "undefined") ? appSettings.maxThrottleLimit : 100
                            onValueChanged: if (typeof appSettings !== "undefined") appSettings.maxThrottleLimit = value
                        }
                        
                        Text { text: "Steering Sensitivity / Expo: " + steeringSlider.value.toFixed(2); color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: steeringSlider
                            Layout.fillWidth: true
                            from: 0.1; to: 2.0
                            value: (typeof appSettings !== "undefined") ? appSettings.steeringSensitivity : 1.0
                            onValueChanged: if (typeof appSettings !== "undefined") appSettings.steeringSensitivity = value
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#555" }
                        Text { text: "Radar & Scanning"; color: "white"; font.bold: true }

                        Text { text: "Radar Point Lifetime (ms)"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            text: (typeof appSettings !== "undefined") ? appSettings.radarPointLifetimeMs.toString() : "5000"
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val) && val > 0 && typeof appSettings !== "undefined") appSettings.radarPointLifetimeMs = val;
                            }
                        }

                        Text { text: "Radar Sweep Speed: " + radarSpeedSlider.value.toFixed(0); color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: radarSpeedSlider
                            Layout.fillWidth: true
                            from: 1; to: 10
                            value: (typeof appSettings !== "undefined") ? appSettings.radarSweepSpeed : 5
                            onValueChanged: {
                                if (typeof appSettings !== "undefined") {
                                    appSettings.radarSweepSpeed = value;
                                }
                                if (typeof commandEmitter !== "undefined") {
                                    commandEmitter.setRadarSweepSpeed(value);
                                }
                            }
                        }
                        
                        Text { text: "Radar Base Angle Offset (deg)"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            text: (typeof appSettings !== "undefined") ? appSettings.sensorBaseAngleDeg.toString() : "0"
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val) && typeof appSettings !== "undefined") appSettings.sensorBaseAngleDeg = val;
                            }
                        }

                        Switch {
                            Layout.fillWidth: true
                            text: "Invert L/R ToF Sensors"
                            checked: (typeof appSettings !== "undefined") ? appSettings.invertTof : false
                            onCheckedChanged: if (typeof appSettings !== "undefined") appSettings.invertTof = checked
                        }
                        
                        Item { Layout.fillHeight: true } // spacer
                    }
                }

                // --- TAB 3: Hardware & Safety ---
                ScrollView {
                    contentWidth: availableWidth
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Text { text: "Hardware & Telemetry"; color: "white"; font.bold: true; Layout.topMargin: 10 }
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

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#555" }
                        Text { text: "Safety & Fusion Overrides"; color: "white"; font.bold: true }
                        
                        Text { text: "Low Battery Warning (Volts): " + battSlider.value.toFixed(1); color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: battSlider
                            Layout.fillWidth: true
                            from: 6.0; to: 12.6
                            value: (typeof appSettings !== "undefined") ? appSettings.lowBatteryWarningVolts : 7.0
                            onValueChanged: if (typeof appSettings !== "undefined") appSettings.lowBatteryWarningVolts = value
                        }
                        
                        Text { text: "Collision Stop Distance (mm): " + collisionSlider.value.toFixed(0); color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: collisionSlider
                            Layout.fillWidth: true
                            from: 50; to: 500; value: 150
                        }
                        
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Failsafe Timeout: 500ms", "Failsafe Timeout: 1s", "Failsafe Timeout: 2s"]
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "Mahony Kp"; color: "gray"; font.pixelSize: 12 }
                                TextField { Layout.fillWidth: true; text: "10.0" }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "Mahony Ki"; color: "gray"; font.pixelSize: 12 }
                                TextField { Layout.fillWidth: true; text: "0.0" }
                            }
                        }

                        Text { text: "Voltage Scale Multiplier"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            text: (typeof appSettings !== "undefined") ? appSettings.voltageScaleMultiplier.toString() : "1.0"
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val) && val > 0 && typeof appSettings !== "undefined") {
                                    appSettings.voltageScaleMultiplier = val;
                                    mainWindow.voltageMultiplier = val; // Also update the local property
                                }
                            }
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#555" }
                        Text { text: "IMU Level Calibration"; color: "white"; font.bold: true }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 15
                            Button {
                                text: "Set Current Level as Zero"
                                Layout.fillWidth: true
                                onClicked: {
                                    if (typeof appSettings !== "undefined" && typeof telemetryClient !== "undefined") {
                                        appSettings.calibrateLevel(telemetryClient.pitch, telemetryClient.roll);
                                    }
                                }
                            }
                            Button {
                                text: "Reset Calibration"
                                Layout.fillWidth: true
                                onClicked: {
                                    if (typeof appSettings !== "undefined") {
                                        appSettings.resetLevelCalibration();
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true } // spacer
                    }
                }

                // --- TAB 4: HUD & Hardware Profile ---
                ScrollView {
                    contentWidth: availableWidth
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        Text { text: "HUD Overlays"; color: "white"; font.bold: true; Layout.topMargin: 10 }
                        
                        Switch {
                            text: "Show Aviation HUD"
                            checked: (typeof appSettings !== "undefined") ? appSettings.displayHudDebug : true
                            onCheckedChanged: if (typeof appSettings !== "undefined") appSettings.displayHudDebug = checked
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "HUD Color:"; color: "white" }
                            ComboBox {
                                id: hudColorCombo
                                Layout.fillWidth: true
                                model: ["#00E5FF", "#00FF00", "#FF1744", "#FFEA00", "#FFFFFF"]
                                currentIndex: (typeof appSettings !== "undefined") ? model.indexOf(appSettings.hudColor) : 0
                                onCurrentIndexChanged: {
                                    if (typeof appSettings !== "undefined" && currentIndex >= 0) {
                                        appSettings.hudColor = model[currentIndex]
                                    }
                                }
                            }
                        }

                        Text { text: "HUD Opacity (" + Math.round(hudOpacitySlider.value * 100) + "%)"; color: "gray"; font.pixelSize: 12 }
                        Slider {
                            id: hudOpacitySlider
                            Layout.fillWidth: true
                            from: 0.1
                            to: 1.0
                            value: (typeof appSettings !== "undefined") ? appSettings.hudOpacity : 0.8
                            onValueChanged: if (typeof appSettings !== "undefined") appSettings.hudOpacity = value
                        }
                        
                        Rectangle { Layout.fillWidth: true; height: 1; color: "#555"; Layout.topMargin: 10; Layout.bottomMargin: 10 }
                        Text { text: "Hardware Metrics Profile"; color: "white"; font.bold: true }

                        Text { text: "Motor Max RPM"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            text: (typeof appSettings !== "undefined") ? appSettings.motorRpm.toString() : "300"
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val) && val > 0 && typeof appSettings !== "undefined") {
                                    appSettings.motorRpm = val;
                                }
                            }
                        }

                        Text { text: "Wheel Size (mm)"; color: "gray"; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true
                            text: (typeof appSettings !== "undefined") ? appSettings.wheelSizeMm.toString() : "60"
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val) && val > 0 && typeof appSettings !== "undefined") {
                                    appSettings.wheelSizeMm = val;
                                }
                            }
                        }

                        Item { Layout.fillHeight: true } // spacer
                    }
                }
            }
        }
    }

    // Floating Script Panel Overlay
    ScriptPanel {
        id: scriptPanelOverlay
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 600)
        height: Math.min(parent.height * 0.8, 500)
        visible: false
        z: 100 // Ensure it's above other elements
        
        onVisibleChanged: {
            if (!visible && typeof rootItem !== "undefined") {
                rootItem.forceActiveFocus();
            }
        }
    }

    // Floating Execute Tool Panel Overlay
    ExecuteToolPanel {
        id: executeToolPanelOverlay
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 500)
        height: Math.min(parent.height * 0.8, 400)
        visible: false
        z: 101 // Above others
        
        onVisibleChanged: {
            if (!visible && typeof rootItem !== "undefined") {
                rootItem.forceActiveFocus();
            }
        }
    }
}
