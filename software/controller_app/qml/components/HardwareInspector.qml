import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#2a2a2a"
    radius: 5
    
    property string profileString: "Awaiting Discovery..." // Bound to DiscoveryWorker
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        
        Text {
            text: "HARDWARE PROFILE"
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#555"
        }
        
        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.profileString
            color: "#00E676"
            font.family: "monospace"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }
}
