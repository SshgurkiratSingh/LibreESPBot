import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#252525"
    border.color: "#444"
    border.width: 1
    radius: 10

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "ROVER SCRIPT ENGINE"
                color: "#00E5FF"
                font.bold: true
                font.pixelSize: 16
                Layout.fillWidth: true
            }
            
            Button {
                text: "Close"
                onClicked: root.visible = false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Text { text: "Examples:"; color: "white" }
            
            ComboBox {
                id: exampleCombo
                Layout.fillWidth: true
                model: ["Custom", "Square Pattern", "Figure 8 (Steering Test)", "Sensor Test (Stop & Scan)"]
                onActivated: {
                    if (currentIndex === 1) {
                        scriptEditor.text = "forward(50)\nwait(1000)\nsteer(100)\nwait(500)\nsteer(0)\nforward(50)\nwait(1000)\nstop()";
                    } else if (currentIndex === 2) {
                        scriptEditor.text = "forward(40)\nsteer(50)\nwait(2000)\nsteer(-50)\nwait(2000)\nstop()";
                    } else if (currentIndex === 3) {
                        scriptEditor.text = "forward(30)\nwait(500)\nstop()\nwait(2000)\nreverse(30)\nwait(500)\nstop()";
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            border.color: "#555"
            border.width: 1
            
            Flickable {
                anchors.fill: parent
                anchors.margins: 5
                
                TextArea.flickable: TextArea {
                    id: scriptEditor
                    color: "#00FF00"
                    font.family: "Monospace"
                    font.pixelSize: 14
                    text: "// Write your script here\n// Commands: forward(p), reverse(p), steer(p), wait(ms), stop()\nforward(50)\nwait(1000)\nstop()"
                    background: null
                }
                ScrollBar.vertical: ScrollBar { }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 15
            
            Button {
                text: typeof scriptEngine !== "undefined" && scriptEngine.isRunning ? "Running..." : "RUN SCRIPT"
                enabled: typeof scriptEngine !== "undefined" && !scriptEngine.isRunning
                Layout.fillWidth: true
                onClicked: {
                    if (typeof scriptEngine !== "undefined") {
                        scriptEngine.runScript(scriptEditor.text)
                    }
                }
                background: Rectangle {
                    color: parent.enabled ? "#00C853" : "#555"
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            Button {
                text: "STOP"
                enabled: typeof scriptEngine !== "undefined" && scriptEngine.isRunning
                Layout.fillWidth: true
                onClicked: {
                    if (typeof scriptEngine !== "undefined") {
                        scriptEngine.stopScript()
                    }
                }
                background: Rectangle {
                    color: parent.enabled ? "#D50000" : "#555"
                    radius: 5
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        
        Text {
            Layout.fillWidth: true
            color: "gray"
            font.pixelSize: 12
            text: {
                if (typeof scriptEngine !== "undefined") {
                    if (scriptEngine.isRunning) return "Executing Line: " + (scriptEngine.currentLine + 1);
                    return "Ready.";
                }
                return "Script Engine Offline.";
            }
        }
        
        Connections {
            target: typeof scriptEngine !== "undefined" ? scriptEngine : null
            function onScriptError(msg) {
                console.warn(msg);
                // Simple error display hack using the editor
                scriptEditor.text = msg + "\n\n" + scriptEditor.text;
            }
        }
    }
}
