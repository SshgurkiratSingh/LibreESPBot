import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "theme"

Window {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    visibility: Qt.platform.os === "android" ? Window.FullScreen : Window.Windowed
    title: qsTr("ESP32 Rover Ground Station")
    color: Theme.backgroundColor // #121212

    // Keyboard Input State
    property int keyThrottle: 0
    property int keySteering: 0

    onKeyThrottleChanged: if (typeof commandEmitter !== "undefined") commandEmitter.updateThrottle(keyThrottle)
    onKeySteeringChanged: if (typeof commandEmitter !== "undefined") commandEmitter.updateSteering(keySteering)

    Item {
        id: rootItem
        anchors.fill: parent
        focus: true // Necessary to capture keyboard events

        Keys.onPressed: (event) => {
            if (event.isAutoRepeat) return;
            if (event.key === Qt.Key_W || event.key === Qt.Key_Up) keyThrottle = 1023;
            else if (event.key === Qt.Key_S || event.key === Qt.Key_Down) keyThrottle = -1023;
            else if (event.key === Qt.Key_A || event.key === Qt.Key_Left) keySteering = -1023;
            else if (event.key === Qt.Key_D || event.key === Qt.Key_Right) keySteering = 1023;
        }

        Keys.onReleased: (event) => {
            if (event.isAutoRepeat) return;
            if (event.key === Qt.Key_W || event.key === Qt.Key_Up || event.key === Qt.Key_S || event.key === Qt.Key_Down) keyThrottle = 0;
            else if (event.key === Qt.Key_A || event.key === Qt.Key_Left || event.key === Qt.Key_D || event.key === Qt.Key_Right) keySteering = 0;
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Left Sidebar: Radar & Hardware Inspector
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: mainWindow.width * 0.25
                color: "#1e1e1e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20

                    Text {
                        text: "RADAR SWEEP"
                        color: Theme.accentCyan
                        font.pixelSize: 18
                        font.bold: true
                    }

                    PolarRadarCanvas {
                        id: radarCanvas
                        Layout.fillWidth: true
                        Layout.preferredHeight: width
                        // points: radarCloud.points // Bind to backend
                    }
                    
                    Item { Layout.fillHeight: true } // spacer

                    HardwareInspector {
                        id: hwInspector
                        Layout.fillWidth: true
                        Layout.preferredHeight: 150
                    }
                }
            }

            // Center Viewport: Video & Horizon
            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                color: "black"
                
                // Placeholder for VideoViewport fills the background
                VideoViewport {
                    anchors.fill: parent
                }

                // Small Artificial Horizon overlay in the top-right corner
                ArtificialHorizon {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 20
                    width: Math.min(150, parent.width * 0.3)
                    height: width
                    // pitch: telemetryClient.pitch
                    // roll: telemetryClient.roll
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
                Layout.fillHeight: true
                Layout.preferredWidth: mainWindow.width * 0.25
                color: "#1e1e1e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20

                    Text {
                        text: "ACTUATION"
                        color: Theme.accentGreen
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
                }
            }
        }
    }

    // Settings Drawer for parameters configuration
    Drawer {
        id: settingsDrawer
        width: 350
        height: mainWindow.height
        edge: Qt.LeftEdge
        
        background: Rectangle {
            color: "#252525"
            border.color: "#444"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15

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

            Text { text: "Manual Connection Override"; color: "white"; font.bold: true }

            TextField {
                id: ipField
                Layout.fillWidth: true
                placeholderText: "Target IP Address (e.g. 192.168.4.1)"
                text: "192.168.4.1"
                color: "white"
            }

            TextField {
                id: portField
                Layout.fillWidth: true
                placeholderText: "Target Port"
                text: "8888"
                color: "white"
                validator: IntValidator { bottom: 1; top: 65535 }
            }

            Button {
                Layout.fillWidth: true
                text: "Connect"
                onClicked: {
                    if (typeof commandEmitter !== "undefined") {
                        commandEmitter.setTargetAddress(ipField.text, parseInt(portField.text))
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#555"
            }

            Text { text: "Rover Configuration"; color: "white"; font.bold: true }

            ComboBox {
                id: modeSelect
                Layout.fillWidth: true
                model: ["Tank Drive (Differential)", "Ackermann Steering"]
            }

            ComboBox {
                id: versionSelect
                Layout.fillWidth: true
                model: ["V1 (ESP8266 + L298N)", "V2 (ESP32 + TB6612)", "V3 (CAN Bus / ODrive)"]
            }

            Item { Layout.fillHeight: true } // Spacer pushes everything up
        }
    }
}
