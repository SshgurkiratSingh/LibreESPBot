import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0a0c10"

    // C++ context objects
    property QtObject aiEngine: null
    property QtObject aiImageStore: null
    property QtObject aiMemoryStore: null
    property bool isRunning: aiEngine ? aiEngine.isRunning : false

    property bool isAskingUser: false
    property string currentQuestion: ""
    property string currentToolCallId: ""
    property string sightLabel: aiEngine ? (aiEngine.supportsVision ? "VISION-CAPABLE MODEL" : "TEXT-ONLY · OPENCV SIGHT") : ""
    property string sightColor: aiEngine ? (aiEngine.supportsVision ? "#888888" : "#ffcc00") : "#555555"

    Connections {
        target: aiEngine
        function onUserAnswerRequested(question, toolCallId) {
            isAskingUser = true
            currentQuestion = question
            currentToolCallId = toolCallId
            taskInput.text = ""
            taskInput.forceActiveFocus()
        }
        function onIsRunningChanged() {
            if (!aiEngine || !aiEngine.isRunning) {
                isAskingUser = false
                currentQuestion = ""
                currentToolCallId = ""
            }
        }
        function onSupportsVisionChanged() {
            if (aiEngine) {
                sightLabel = aiEngine.supportsVision ? "VISION-CAPABLE MODEL" : "TEXT-ONLY · OPENCV SIGHT"
                sightColor = aiEngine.supportsVision ? "#888888" : "#ffcc00"
            }
        }
    }

    // ── scanline overlay ──────────────────────────────────────────────────────
    Canvas {
        anchors.fill: parent
        opacity: 0.03
        onPaint: {
            var ctx = getContext("2d");
            ctx.fillStyle = "#00ff88";
            for (var y = 0; y < height; y += 4) {
                ctx.fillRect(0, y, width, 1);
            }
        }
    }

    // ── grid ──────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ── TOP BAR ──────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            height: 48
            spacing: 16

            // Blinking status indicator
            Rectangle {
                width: 10; height: 10; radius: 5
                color: isRunning ? "#00ff88" : "#ff4444"
                SequentialAnimation on opacity {
                    running: isRunning
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 600 }
                    NumberAnimation { to: 1.0; duration: 600 }
                }
                opacity: isRunning ? 1 : 0.5
            }

            Text {
                text: "ROVER AI MISSION CONTROL"
                color: "#00ff88"
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 4
                font.family: "monospace"
            }

            // Status badge
            Rectangle {
                width: statusLabel.implicitWidth + 24
                height: 28
                radius: 4
                color: isRunning ? "#0d3320" : "#2a0a0a"
                border.color: isRunning ? "#00ff88" : "#ff4444"
                border.width: 1
                Text {
                    id: statusLabel
                    anchors.centerIn: parent
                    text: isRunning ? "MISSION ACTIVE" : "STANDBY"
                    color: isRunning ? "#00ff88" : "#ff4444"
                    font.bold: true
                    font.pixelSize: 11
                    font.letterSpacing: 2
                    font.family: "monospace"
                }
            }

// Sight-mode badge (DeepSeek is text-only; OpenCV supplies "sight")
            Text {
                text: sightLabel
                color: sightColor
                font.pixelSize: 10
                font.bold: true
                font.family: "monospace"
            }
            Item { Layout.fillWidth: true }

            // API Config
            Text { text: "KEY:"; color: "#555"; font.pixelSize: 11; font.family: "monospace" }
            Rectangle {
                width: 260; height: 30
                color: "#0d1117"; radius: 4
                border.color: apiKeyInput.activeFocus ? "#00ff88" : "#2a2a2a"
                TextInput {
                    id: apiKeyInput
                    anchors.fill: parent; anchors.margins: 8
                    color: "#00ff88"
                    font.pixelSize: 13; font.family: "monospace"
                    echoMode: TextInput.Password
                    text: (typeof appSettings !== "undefined") ? appSettings.aiApiKey : ""
                    onTextChanged: if (typeof appSettings !== "undefined") appSettings.aiApiKey = text
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                }
            }

            Text { text: "MODEL:"; color: "#555"; font.pixelSize: 11; font.family: "monospace" }
            Rectangle {
                width: 160; height: 30
                color: "#0d1117"; radius: 4
                border.color: modelInput.activeFocus ? "#00ff88" : "#2a2a2a"
                TextInput {
                    id: modelInput
                    anchors.fill: parent; anchors.margins: 8
                    color: "#00ff88"
                    font.pixelSize: 13; font.family: "monospace"
                    text: (typeof appSettings !== "undefined") ? appSettings.aiModelName : "deepseek-flash"
                    onTextChanged: if (typeof appSettings !== "undefined") appSettings.aiModelName = text
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                }
            }
        }
// API base URL (DeepSeek = https://api.deepseek.com)
            Text { text: "URL:"; color: "#555"; font.pixelSize: 11; font.family: "monospace" }
            Rectangle {
                width: 230; height: 30
                color: "#0d1117"; radius: 4
                border.color: urlInput.activeFocus ? "#00ff88" : "#2a2a2a"
                TextInput {
                    id: urlInput
                    anchors.fill: parent; anchors.margins: 8
                    color: "#00ff88"
                    font.pixelSize: 12; font.family: "monospace"
                    text: (typeof appSettings !== "undefined") ? appSettings.aiBaseUrl : "https://api.deepseek.com"
                    onTextChanged: if (typeof appSettings !== "undefined") appSettings.aiBaseUrl = text
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                }
            }

            // Standing user instructions (persisted, injected into every mission)
            Column {
                spacing: 2
                Text { text: "ORDERS"; color: "#555"; font.pixelSize: 9; font.family: "monospace"; horizontalAlignment: Text.AlignHCenter; anchors.horizontalCenter: parent.horizontalCenter }
                Rectangle {
                    width: 260; height: 30
                    color: "#0d1117"; radius: 4
                    border.color: ordersInput.activeFocus ? "#00ff88" : "#2a2a2a"
                    TextInput {
                        id: ordersInput
                        anchors.fill: parent; anchors.margins: 8
                        color: "#ffcc66"
                        font.pixelSize: 12; font.family: "monospace"
                        text: (typeof appSettings !== "undefined") ? appSettings.aiUserInstruction : ""
                        onTextChanged: if (typeof appSettings !== "undefined") appSettings.aiUserInstruction = text
                        verticalAlignment: TextInput.AlignVCenter
                        clip: true
                    }
                }
            }

            // Force vision-capable (for custom OpenAI-compatible gateways)
            Column {
                spacing: 2
                Text { text: "VISION"; color: "#555"; font.pixelSize: 9; font.family: "monospace"; horizontalAlignment: Text.AlignHCenter; anchors.horizontalCenter: parent.horizontalCenter }
                Switch {
                    id: visionSwitch
                    checked: (typeof appSettings !== "undefined") ? appSettings.aiSupportsVision : false
                    onCheckedChanged: if (typeof appSettings !== "undefined") appSettings.aiSupportsVision = checked
                    scale: 0.8
                }
            }

        // ── SEPARATOR ─────────────────────────────────────────────────────────
        Rectangle { Layout.fillWidth: true; height: 1; color: "#0d2a1a" }

        // ── MAIN PANES ────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ── LEFT: Thinking ────────────────────────────────────────────────
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.38
                color: "#050709"
                radius: 6
                border.color: "#0d2a1a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Panel header
                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle { width: 3; height: 14; color: "#00ff88"; radius: 1 }
                        Text { text: "CHAIN OF THOUGHT"; color: "#00ff88"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2; font.family: "monospace" }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: aiEngine ? (aiEngine.thinkingLog.length + " chars") : "0 chars"
                            color: "#333"; font.pixelSize: 10; font.family: "monospace"
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#0d2a1a" }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        clip: true

                        TextArea {
                            id: thinkArea
                            readOnly: true
                            text: aiEngine ? aiEngine.thinkingLog : ""
                            color: "#6a8f6a"
                            font.pixelSize: 12
                            font.family: "monospace"
                            wrapMode: Text.Wrap
                            background: null
                            padding: 4
                            onTextChanged: {
                                // Auto-scroll
                            }
                        }
                    }
                // ── PAST THINKING (history of completed reasoning rounds) ──
                RowLayout {
                    Layout.fillWidth: true
                    Rectangle { width: 3; height: 14; color: "#88aa88"; radius: 1 }
                    Text { text: "PAST THINKING"; color: "#88aa88"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 2; font.family: "monospace" }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: aiEngine ? (aiEngine.pastThinkingLog.length + " chars") : ""
                        color: "#333"; font.pixelSize: 9; font.family: "monospace"
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#0d2a1a" }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    clip: true
                    TextArea {
                        readOnly: true
                        text: aiEngine ? aiEngine.pastThinkingLog : ""
                        color: "#5a7a5a"
                        font.pixelSize: 11
                        font.family: "monospace"
                        wrapMode: Text.Wrap
                        background: null
                        padding: 4
                        placeholderText: "Past reasoning will appear here after the first round."
                    }
                }

                }
            }

            // ── CENTER: Response ──────────────────────────────────────────────
            Rectangle {
                Layout.fillHeight: true
                Layout.fillWidth: true
                color: "#050709"
                radius: 6
                border.color: "#0d2a1a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle { width: 3; height: 14; color: "#00e5ff"; radius: 1 }
                        Text { text: "AI RESPONSE"; color: "#00e5ff"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2; font.family: "monospace" }
                        Item { Layout.fillWidth: true }
                        // Typing cursor blink
                        Text {
                            text: "▮"
                            color: "#00e5ff"
                            font.pixelSize: 14
                            visible: isRunning
                            SequentialAnimation on opacity {
                                running: isRunning
                                loops: Animation.Infinite
                                NumberAnimation { to: 0; duration: 500 }
                                NumberAnimation { to: 1; duration: 500 }
                            }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#0a1e2a" }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TextArea {
                            id: responseArea
                            readOnly: true
                            text: aiEngine ? (aiEngine.responseLog || "") : ""
                            color: "#d0e8ff"
                            font.pixelSize: 13
                            font.family: "monospace"
                            wrapMode: Text.Wrap
                            background: null
                            padding: 4
                        }
                    }

                    // Error bar — only visible on error
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        radius: 4
                        color: "#2a0808"
                        border.color: "#ff4444"
                        border.width: 1
                        visible: aiEngine ? (aiEngine.errorLog || "").length > 0 : false

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            Text { text: "ERR"; color: "#ff4444"; font.bold: true; font.pixelSize: 11; font.family: "monospace" }
                            Text {
                                Layout.fillWidth: true
                                text: aiEngine ? (aiEngine.errorLog || "").split("\n").filter(function(l){ return l.length > 0; }).pop() || "" : ""
                                color: "#ff8888"
                                font.pixelSize: 11
                                font.family: "monospace"
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // ── RIGHT: Tool Log ───────────────────────────────────────────────
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.26
                color: "#050709"
                radius: 6
                border.color: "#0d2a1a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle { width: 3; height: 14; color: "#ff9800"; radius: 1 }
                        Text { text: "TOOL CALLS"; color: "#ff9800"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 2; font.family: "monospace" }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#2a1800" }

                    ListView {
                        id: toolLogView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: aiEngine ? aiEngine.toolLogModel : null
                        clip: true
                        spacing: 6

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: toolDelegateCol.implicitHeight + 16
                            color: "#0d0f0a"
                            radius: 4
                            border.color: model.status === "OK" || model.status === "SUCCESS" ? "#1a3a1a" : "#3a1a1a"
                            border.width: 1

                            Column {
                                id: toolDelegateCol
                                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 8 }
                                spacing: 3

                                RowLayout {
                                    width: parent.width
                                    Text {
                                        text: model.toolName
                                        color: "#ff9800"
                                        font.bold: true
                                        font.pixelSize: 12
                                        font.family: "monospace"
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: model.status
                                        color: model.status === "OK" || model.status === "SUCCESS" ? "#00ff88" : "#ff4444"
                                        font.pixelSize: 10
                                        font.family: "monospace"
                                    }
                                }
                                Text {
                                    width: parent.width
                                    text: model.time
                                    color: "#444"
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                }
                                Text {
                                    width: parent.width
                                    text: model.argsText
                                    color: "#8a8a8a"
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 4
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "No tool calls yet"
                            color: "#2a2a2a"
                            font.pixelSize: 12
                            font.family: "monospace"
                            visible: toolLogView.count === 0
                        }
                    }
                }
            }
        }

        // ── MEMORY STRIP (save_image_to_memory items) ─────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 128
            color: "#050709"
            radius: 6
            border.color: "#1c2a4a"
            border.width: 1
            visible: aiImageStore !== null && aiImageStore.memoryModel !== null

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: "MEMORY"
                    color: "#88aaff"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 2
                    font.family: "monospace"
                    Layout.alignment: Qt.AlignVCenter
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 8
                    model: aiImageStore !== null ? aiImageStore.memoryModel : null
                    clip: true

                    delegate: Rectangle {
                        width: 200; height: 112
                        radius: 4
                        color: "#0a0c12"
                        border.color: "#2a3a5a"
                        border.width: 1
                        clip: true

                        Column {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4
                            Image {
                                width: parent.width; height: 70
                                source: model.filepath
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                            Text {
                                width: parent.width
                                text: model.key
                                color: "#88aaff"
                                font.bold: true
                                font.pixelSize: 10
                                font.family: "monospace"
                            }
                            Text {
                                width: parent.width
                                text: model.description.split("
")[0]
                                color: "#5e8f8f"
                                font.pixelSize: 9
                                font.family: "monospace"
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No memory images"
                        color: "#2a2a2a"
                        font.pixelSize: 11
                        font.family: "monospace"
                        visible: parent.count === 0
                    }
                }
            }
        }


        // ── NOTES STRIP (persistent text memory: remember / recall_memory) ────
        Rectangle {
            Layout.fillWidth: true
            height: 96
            color: "#050709"
            radius: 6
            border.color: "#2a2a3a"
            border.width: 1
            visible: aiMemoryStore !== null

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: "NOTES"
                    color: "#cc88ff"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 2
                    font.family: "monospace"
                    Layout.alignment: Qt.AlignVCenter
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 8
                    clip: true
                    model: aiMemoryStore !== null ? aiMemoryStore : null

                    delegate: Rectangle {
                        width: 280; height: 80
                        radius: 4
                        color: "#0a0c12"
                        border.color: "#3a2a5a"
                        border.width: 1
                        clip: true

                        Column {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4
                            Text {
                                width: parent.width
                                text: model.time
                                color: "#554466"
                                font.pixelSize: 9
                                font.family: "monospace"
                            }
                            Text {
                                width: parent.width
                                height: 44
                                text: model.text
                                color: "#ccbbdd"
                                font.pixelSize: 10
                                font.family: "monospace"
                                wrapMode: Text.Wrap
                                elide: Text.ElideRight
                                maximumLineCount: 3
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No stored notes — agent can use the 'remember' tool"
                        color: "#2a2a2a"
                        font.pixelSize: 11
                        font.family: "monospace"
                        visible: parent.count === 0
                    }
                }

                Button {
                    text: "CLEAR"
                    Layout.alignment: Qt.AlignVCenter
                    font.pixelSize: 10
                    font.family: "monospace"
                    onClicked: if (aiMemoryStore !== null) aiMemoryStore.clear()
                }
            }
        }


        // ── IMAGE STRIP ───────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 90
            color: "#050709"
            radius: 6
            border.color: "#0d2a1a"
            border.width: 1
            visible: aiImageStore !== null

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: "CAPTURES"
                    color: "#333"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 2
                    font.family: "monospace"
                    Layout.alignment: Qt.AlignVCenter
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 8
                    model: aiImageStore
                    clip: true

                    delegate: Rectangle {
                        width: 100; height: 70
                        radius: 4
                        color: "#0a0c10"
                        border.color: "#1a3a1a"
                        border.width: 1
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: model.filepath
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width; height: 18
                            color: "#aa000000"
                            Text {
                                anchors.centerIn: parent
                                text: Qt.formatDateTime(model.timestamp, "hh:mm:ss")
                                color: "#00ff88"; font.pixelSize: 9; font.family: "monospace"
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No captures"
                        color: "#2a2a2a"
                        font.pixelSize: 11
                        font.family: "monospace"
                        visible: parent.count === 0
                    }
                }
            }
        }

        // ── BOTTOM COMMAND BAR ────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#050709"
            radius: 6
            border.color: isAskingUser ? "#ffcc00" : (isRunning ? "#00ff88" : "#0d2a1a")
            border.width: 1

            Behavior on border.color { ColorAnimation { duration: 300 } }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                Text {
                    text: isAskingUser ? "?_" : ">_"
                    color: isAskingUser ? "#ffcc00" : "#00ff88"
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "monospace"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"
                    
                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: currentQuestion
                        color: "#555500"
                        font.pixelSize: 14
                        font.family: "monospace"
                        visible: isAskingUser && taskInput.text.length === 0
                    }

                    TextInput {
                        id: taskInput
                        anchors.fill: parent
                        color: isAskingUser ? "#ffdd55" : "#d0e8ff"
                        font.pixelSize: 14
                        font.family: "monospace"
                        verticalAlignment: TextInput.AlignVCenter
                        text: (typeof appSettings !== "undefined" && !isAskingUser) ? appSettings.aiSystemPrompt : ""
                        onTextChanged: if (typeof appSettings !== "undefined" && !isAskingUser) appSettings.aiSystemPrompt = text
                        clip: true
                        Keys.onReturnPressed: {
                            if (isAskingUser && aiEngine) {
                                let ans = taskInput.text;
                                isAskingUser = false;
                                currentQuestion = "";
                                taskInput.text = "";
                                aiEngine.provideUserAnswer(currentToolCallId, ans);
                            } else if (!isRunning && aiEngine) {
                                aiEngine.startMission(taskInput.text);
                            }
                        }
                    }
                }

                // LAUNCH button
                Rectangle {
                    width: 140; height: 40; radius: 4
                    color: isAskingUser ? (launchMa.containsMouse ? "#ccaa00" : "#886600") : (isRunning ? "#0d2a1a" : (launchMa.containsMouse ? "#00aa55" : "#006633"))
                    border.color: isAskingUser ? "#ffcc00" : "#00ff88"; border.width: 1
                    enabled: !isRunning || isAskingUser
                    opacity: (!isRunning || isAskingUser) ? 1 : 0.5
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: isAskingUser ? "SUBMIT ANSWER" : (isRunning ? "TRANSMITTING..." : "LAUNCH MISSION")
                        color: isAskingUser ? "#ffcc00" : "#00ff88"; font.bold: true; font.pixelSize: 11
                        font.letterSpacing: 1; font.family: "monospace"
                    }
                    MouseArea {
                        id: launchMa
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: !isRunning || isAskingUser
                        onClicked: {
                            if (isAskingUser && aiEngine) {
                                let ans = taskInput.text;
                                isAskingUser = false;
                                currentQuestion = "";
                                taskInput.text = "";
                                aiEngine.provideUserAnswer(currentToolCallId, ans);
                            } else if (!isRunning && aiEngine) {
                                aiEngine.startMission(taskInput.text);
                            }
                        }
                    }
                }

                // ABORT button
                Rectangle {
                    width: 80; height: 40; radius: 4
                    color: isRunning ? (abortMa.containsMouse ? "#aa2222" : "#880000") : "#1a0000"
                    border.color: "#ff4444"; border.width: 1
                    enabled: isRunning
                    opacity: isRunning ? 1 : 0.3
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: "ABORT"
                        color: "#ff4444"; font.bold: true; font.pixelSize: 11
                        font.letterSpacing: 1; font.family: "monospace"
                    }
                    MouseArea {
                        id: abortMa
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: isRunning
                        onClicked: if (aiEngine) aiEngine.stopMission()
                    }
                }

                // Auto-loop toggle
                Column {
                    spacing: 2
                    Text { text: "AUTO"; color: "#555"; font.pixelSize: 9; font.family: "monospace"; horizontalAlignment: Text.AlignHCenter; anchors.horizontalCenter: parent.horizontalCenter }
                    Switch {
                        id: autoLoopSwitch
                        checked: (typeof appSettings !== "undefined") ? appSettings.aiAutoLoop : false
                        onCheckedChanged: if (typeof appSettings !== "undefined") appSettings.aiAutoLoop = checked
                        scale: 0.8
                    }
                }
            }
        }
    }
}
