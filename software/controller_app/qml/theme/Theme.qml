pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color backgroundColor: "#121212"
    readonly property color panelColor: "#1e1e1e"
    readonly property color accentCyan: "#00E5FF"
    readonly property color accentGreen: "#00E676"
    readonly property color alertRed: "#FF1744"
    readonly property color warningOrange: "#FF9100"
    
    readonly property int radarRingSmall: 50
    readonly property int radarRingMedium: 100
    readonly property int radarRingLarge: 150
    readonly property int radarRingMax: 200
}
