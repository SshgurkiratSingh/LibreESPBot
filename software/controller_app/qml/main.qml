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
    title: qsTr("ESP32 Rover Ground Station")
    color: Theme.backgroundColor // #121212

    Theme { id: theme }

    // Backend context properties assumed available: telemetryClient, commandEmitter, discoveryWorker, radarCloud

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left Sidebar: Radar & Hardware Inspector
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 350
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
            
            // Placeholder for VideoViewport
            VideoViewport {
                anchors.fill: parent
            }

            ArtificialHorizon {
                anchors.centerIn: parent
                // pitch: telemetryClient.pitch
                // roll: telemetryClient.roll
            }
        }

        // Right Sidebar: Controls
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 350
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

                RowLayout {
                    Layout.fillWidth: true
                    VirtualJoystick {
                        id: leftJoystick
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 150
                        axisXEnabled: false
                        // onAxisYChanged: commandEmitter.updateThrottle(axisY)
                    }
                    VirtualJoystick {
                        id: rightJoystick
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 150
                        axisYEnabled: false
                        // onAxisXChanged: commandEmitter.updateSteering(axisX)
                    }
                }

                Item { Layout.fillHeight: true }
                
                // Automations Drawer Toggles
                Switch { text: "Auto Emergency Brake (AEB)" }
                Switch { text: "APF Collision Avoidance" }
                Switch { text: "Radar Sweep" }
            }
        }
    }
}
