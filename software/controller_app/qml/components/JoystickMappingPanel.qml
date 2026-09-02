import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Dialog {
    id: joystickDialog
    title: "Gamepad Mapping Settings"
    width: 400
    height: 500
    modal: true
    standardButtons: Dialog.Close
    
    // Position at center of window
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    property var currentMapping: typeof joystickHandler !== "undefined" ? joystickHandler.mapping : ({})
    property string listeningAction: ""

    Connections {
        target: typeof joystickHandler !== "undefined" ? joystickHandler : null
        function onLastLearnedInputChanged() {
            if (listeningAction !== "" && joystickHandler.isLearning) {
                var map = currentMapping;
                map[listeningAction] = joystickHandler.lastLearnedInput;
                joystickHandler.mapping = map; // Update C++ mapping
                currentMapping = map; // Update local mapping
                
                listeningAction = "";
                joystickHandler.isLearning = false;
                learningOverlay.visible = false;
                
                // Save to disk
                joystickHandler.saveMapping();
            }
        }
        
        function onConnectedChanged() {
            deviceStatus.text = joystickHandler.connected ? "Connected: " + joystickHandler.deviceName : "No Gamepad Found"
            deviceStatus.color = joystickHandler.connected ? Material.color(Material.Green) : Material.color(Material.Red)
        }
    }

    Rectangle {
        id: learningOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.8)
        z: 100
        visible: false
        
        ColumnLayout {
            anchors.centerIn: parent
            Text {
                text: "Press any button or move any axis for:\n" + listeningAction
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "Cancel"
                Layout.alignment: Qt.AlignHCenter
                onClicked: {
                    listeningAction = ""
                    joystickHandler.isLearning = false
                    learningOverlay.visible = false
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Text {
            id: deviceStatus
            text: (typeof joystickHandler !== "undefined" && joystickHandler.connected) ? "Connected: " + joystickHandler.deviceName : "No Gamepad Found"
            color: (typeof joystickHandler !== "undefined" && joystickHandler.connected) ? Material.color(Material.Green) : Material.color(Material.Red)
            font.bold: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ColumnLayout {
                width: parent.width - 20
                spacing: 10
                
                Repeater {
                    model: ["Throttle", "Steering", "Brake", "Reverse", "Radar", "SpeedMode"]
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: modelData
                            color: "white"
                            Layout.fillWidth: true
                        }
                        
                        Button {
                            text: currentMapping[modelData] ? currentMapping[modelData] : "Unmapped"
                            onClicked: {
                                listeningAction = modelData;
                                if (typeof joystickHandler !== "undefined") {
                                    joystickHandler.isLearning = true;
                                    learningOverlay.visible = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
