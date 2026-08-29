import QtQuick 2.15
import QtMultimedia

Rectangle {
    id: root
    color: "#0a0a0a"

    property string streamUrl: ""
    property bool isConnected: false
    
    // Connect to discovery worker to dynamically set the stream URL
    Connections {
        target: typeof discoveryWorker !== "undefined" ? discoveryWorker : null
        function onCameraDiscovered(ip) {
            console.log("VideoViewport configuring stream for camera IP: " + ip)
            root.streamUrl = "http://" + ip + "/"
            startStream()
        }
    }

    MediaPlayer {
        id: player
        videoOutput: videoOut
        source: root.streamUrl
        
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.PlayingState) {
                root.isConnected = true
                watchdogTimer.restart()
            }
        }
        
        onErrorOccurred: (error, errorString) => {
            console.warn("Video stream error:", errorString)
            root.isConnected = false
        }
    }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: root.isConnected
    }

    // Watchdog timer to detect stuck frames or connection drop
    Timer {
        id: watchdogTimer
        interval: 3000
        repeat: true
        running: true
        onTriggered: {
            if (root.streamUrl !== "") {
                if (player.playbackState !== MediaPlayer.PlayingState || !root.isConnected) {
                    console.log("Video stream appears stuck/disconnected, attempting reconnect...")
                    root.isConnected = false
                    player.stop()
                    // Append timestamp to bust cache
                    player.source = root.streamUrl + "?t=" + new Date().getTime()
                    player.play()
                }
            }
        }
    }

    function startStream() {
        if (root.streamUrl !== "") {
            player.source = root.streamUrl + "?t=" + new Date().getTime()
            player.play()
        }
    }

    Text {
        anchors.centerIn: parent
        width: parent.width * 0.9
        wrapMode: Text.WordWrap
        text: root.streamUrl === "" 
              ? "VIDEO STREAM OFFLINE\n(Waiting for ESP32-CAM mDNS Discovery)" 
              : "RECONNECTING TO CAMERA...\n" + root.streamUrl
        color: "#FF1744"
        font.pixelSize: 16
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        visible: !root.isConnected
    }
}
