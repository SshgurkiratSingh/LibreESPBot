#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class AiToolSpec {
public:
    static QJsonArray getToolDefinitions() {
        QJsonArray tools;

        // 1. run_script
        QJsonObject runScriptTool;
        runScriptTool["type"] = "function";
        QJsonObject runScriptFunc;
        runScriptFunc["name"] = "run_script";
        runScriptFunc["description"] = "Executes a movement or action script on the rover. Commands: forward(p), reverse(p), steer(p), wait(ms), stop(), turn_to(deg), headlight(n). Separate commands with newlines.";
        QJsonObject runScriptParams;
        runScriptParams["type"] = "object";
        QJsonObject runScriptProps;
        QJsonObject scriptProp;
        scriptProp["type"] = "string";
        scriptProp["description"] = "The script code to execute (e.g., 'forward(30)\\nwait(1000)\\nstop()').";
        runScriptProps["script"] = scriptProp;
        runScriptParams["properties"] = runScriptProps;
        QJsonArray runScriptReq;
        runScriptReq.append("script");
        runScriptParams["required"] = runScriptReq;
        runScriptFunc["parameters"] = runScriptParams;
        runScriptTool["function"] = runScriptFunc;
        tools.append(runScriptTool);

        // 2. stop_rover
        QJsonObject stopRoverTool;
        stopRoverTool["type"] = "function";
        QJsonObject stopRoverFunc;
        stopRoverFunc["name"] = "stop_rover";
        stopRoverFunc["description"] = "Immediately halts all running scripts and stops the rover motors.";
        QJsonObject emptyParams;
        emptyParams["type"] = "object";
        emptyParams["properties"] = QJsonObject();
        stopRoverFunc["parameters"] = emptyParams;
        stopRoverTool["function"] = stopRoverFunc;
        tools.append(stopRoverTool);

        // 3. save_image
        QJsonObject saveImageTool;
        saveImageTool["type"] = "function";
        QJsonObject saveImageFunc;
        saveImageFunc["name"] = "save_image";
        saveImageFunc["description"] = "Saves the current camera frame to local disk. Returns a text description of what the saved frame actually shows, so you always get visual evidence back.";
        QJsonObject saveImageParams;
        saveImageParams["type"] = "object";
        QJsonObject saveImageProps;
        QJsonObject labelProp;
        labelProp["type"] = "string";
        labelProp["description"] = "A short descriptive label for the image file name (e.g., 'door', 'hallway').";
        saveImageProps["label"] = labelProp;
        saveImageParams["properties"] = saveImageProps;
        QJsonArray saveImageReq;
        saveImageReq.append("label");
        saveImageParams["required"] = saveImageReq;
        saveImageFunc["parameters"] = saveImageParams;
        saveImageTool["function"] = saveImageFunc;
        tools.append(saveImageTool);

        // 4. save_image_to_memory
        QJsonObject saveMemTool;
        saveMemTool["type"] = "function";
        QJsonObject saveMemFunc;
        saveMemFunc["name"] = "save_image_to_memory";
        saveMemFunc["description"] = "Stores the current frame in memory under a key and returns a text description of what it shows. Recall it later with recall_image. Use this to remember a visual (e.g. a hallway, a door) for comparison.";
        QJsonObject saveMemParams;
        saveMemParams["type"] = "object";
        QJsonObject saveMemProps;
        QJsonObject keyProp;
        keyProp["type"] = "string";
        keyProp["description"] = "A unique key to store this image under (e.g., 'start_position').";
        saveMemProps["key"] = keyProp;
        saveMemParams["properties"] = saveMemProps;
        QJsonArray saveMemReq;
        saveMemReq.append("key");
        saveMemParams["required"] = saveMemReq;
        saveMemFunc["parameters"] = saveMemParams;
        saveMemTool["function"] = saveMemFunc;
        tools.append(saveMemTool);

        // 5. recall_image
        QJsonObject recallTool;
        recallTool["type"] = "function";
        QJsonObject recallFunc;
        recallFunc["name"] = "recall_image";
        recallFunc["description"] = "Recalls a previously saved in-memory image and returns a text description of what it showed (the model cannot view raw pixels). Optionally embeds the actual image when the selected model is vision-capable.";
        QJsonObject recallParams;
        recallParams["type"] = "object";
        QJsonObject recallProps;
        recallProps["key"] = keyProp; // Reuse keyProp from above
        recallParams["properties"] = recallProps;
        QJsonArray recallReq;
        recallReq.append("key");
        recallParams["required"] = recallReq;
        recallFunc["parameters"] = recallParams;
        recallTool["function"] = recallFunc;
        tools.append(recallTool);

        // 5b. describe_view — local "sight" for text-only models
        QJsonObject viewTool;
        viewTool["type"] = "function";
        QJsonObject viewFunc;
        viewFunc["name"] = "describe_view";
        viewFunc["description"] = "Provides a text description of what the camera currently sees: lighting, dominant colors, contrast, detail, faces and composition. Use this to \"see\" the environment — text-only models cannot view raw image files.";
        QJsonObject viewParams;
        viewParams["type"] = "object";
        QJsonObject viewProps;
        QJsonObject viewKeyProp;
        viewKeyProp["type"] = "string";
        viewKeyProp["description"] = "Optional memory key to store this scene description under for later recall (e.g. 'doorframe').";
        viewProps["key"] = viewKeyProp;
        viewParams["properties"] = viewProps;
        viewFunc["parameters"] = viewParams;
        viewTool["function"] = viewFunc;
        tools.append(viewTool);

        // 6. set_headlight
        QJsonObject headlightTool;
        headlightTool["type"] = "function";
        QJsonObject headlightFunc;
        headlightFunc["name"] = "set_headlight";
        headlightFunc["description"] = "Sets the rover's headlight intensity mode (0-6). 0 is off, 6 is max brightness.";
        QJsonObject headlightParams;
        headlightParams["type"] = "object";
        QJsonObject headlightProps;
        QJsonObject modeProp;
        modeProp["type"] = "integer";
        modeProp["description"] = "Intensity mode from 0 (off) to 6 (max).";
        headlightProps["mode"] = modeProp;
        headlightParams["properties"] = headlightProps;
        QJsonArray headlightReq;
        headlightReq.append("mode");
        headlightParams["required"] = headlightReq;
        headlightFunc["parameters"] = headlightParams;
        headlightTool["function"] = headlightFunc;
        tools.append(headlightTool);

        // 7. set_servo
        QJsonObject servoTool;
        servoTool["type"] = "function";
        QJsonObject servoFunc;
        servoFunc["name"] = "set_servo";
        servoFunc["description"] = "Directly positions the camera pan servo.";
        QJsonObject servoParams;
        servoParams["type"] = "object";
        QJsonObject servoProps;
        QJsonObject angleProp;
        angleProp["type"] = "integer";
        angleProp["description"] = "Pan angle in degrees (-90 to +90). 0 is center.";
        servoProps["angle"] = angleProp;
        servoParams["properties"] = servoProps;
        QJsonArray servoReq;
        servoReq.append("angle");
        servoParams["required"] = servoReq;
        servoFunc["parameters"] = servoParams;
        servoTool["function"] = servoFunc;
        tools.append(servoTool);

        // 8. sweep_radar
        QJsonObject sweepTool;
        sweepTool["type"] = "function";
        QJsonObject sweepFunc;
        sweepFunc["name"] = "sweep_radar";
        sweepFunc["description"] = "Starts an automatic servo radar sweep at the given speed.";
        QJsonObject sweepParams;
        sweepParams["type"] = "object";
        QJsonObject sweepProps;
        QJsonObject speedProp;
        speedProp["type"] = "integer";
        speedProp["description"] = "Sweep speed from 1 (slowest) to 10 (fastest).";
        sweepProps["speed"] = speedProp;
        sweepParams["properties"] = sweepProps;
        QJsonArray sweepReq;
        sweepReq.append("speed");
        sweepParams["required"] = sweepReq;
        sweepFunc["parameters"] = sweepParams;
        sweepTool["function"] = sweepFunc;
        tools.append(sweepTool);

        // 9. wait_for_telemetry
        QJsonObject waitTool;
        waitTool["type"] = "function";
        QJsonObject waitFunc;
        waitFunc["name"] = "wait_for_telemetry";
        waitFunc["description"] = "Blocks and waits until a sensor expression is true or timeout is reached (NOT YET IMPLEMENTED).";
        QJsonObject waitParams;
        waitParams["type"] = "object";
        QJsonObject waitProps;
        QJsonObject condProp;
        condProp["type"] = "string";
        condProp["description"] = "Condition expression (e.g., 'tof1 < 300').";
        waitProps["condition"] = condProp;
        QJsonObject timeoutProp;
        timeoutProp["type"] = "integer";
        timeoutProp["description"] = "Timeout in milliseconds.";
        waitProps["timeout_ms"] = timeoutProp;
        waitParams["properties"] = waitProps;
        QJsonArray waitReq;
        waitReq.append("condition");
        waitReq.append("timeout_ms");
        waitParams["required"] = waitReq;
        waitFunc["parameters"] = waitParams;
        waitTool["function"] = waitFunc;
        tools.append(waitTool);

        // 10. speak
        QJsonObject speakTool;
        speakTool["type"] = "function";
        QJsonObject speakFunc;
        speakFunc["name"] = "speak";
        speakFunc["description"] = "Displays a speech bubble from the AI on the UI.";
        QJsonObject speakParams;
        speakParams["type"] = "object";
        QJsonObject speakProps;
        QJsonObject textProp;
        textProp["type"] = "string";
        textProp["description"] = "The text to speak.";
        speakProps["text"] = textProp;
        speakParams["properties"] = speakProps;
        QJsonArray speakReq;
        speakReq.append("text");
        speakParams["required"] = speakReq;
        speakFunc["parameters"] = speakParams;
        speakTool["function"] = speakFunc;
        tools.append(speakTool);

        // 11. log_note
        QJsonObject logTool;
        logTool["type"] = "function";
        QJsonObject logFunc;
        logFunc["name"] = "log_note";
        logFunc["description"] = "Logs a timestamped note to the session log.";
        QJsonObject logParams;
        logParams["type"] = "object";
        QJsonObject logProps;
        logProps["text"] = textProp; // Reuse textProp
        logParams["properties"] = logProps;
        QJsonArray logReq;
        logReq.append("text");
        logParams["required"] = logReq;
        logFunc["parameters"] = logParams;
        logTool["function"] = logFunc;
        tools.append(logTool);

        // 12. take_panorama
        QJsonObject panoTool;
        panoTool["type"] = "function";
        QJsonObject panoFunc;
        panoFunc["name"] = "take_panorama";
        panoFunc["description"] = "Invokes the Panorama Builder to capture a 360 degree image.";
        panoFunc["parameters"] = emptyParams;
        panoTool["function"] = panoFunc;
        tools.append(panoTool);

        // 13. ask_user
        QJsonObject askTool;
        askTool["type"] = "function";
        QJsonObject askFunc;
        askFunc["name"] = "ask_user";
        askFunc["description"] = "Pauses AI execution and asks the user a clarifying question.";
        QJsonObject askParams;
        askParams["type"] = "object";
        QJsonObject askProps;
        QJsonObject qProp;
        qProp["type"] = "string";
        qProp["description"] = "The question to ask the user.";
        askProps["question"] = qProp;
        askParams["properties"] = askProps;
        QJsonArray askReq;
        askReq.append("question");
        askParams["required"] = askReq;
        askFunc["parameters"] = askParams;
        askTool["function"] = askFunc;
        tools.append(askTool);

        // 14. loop_enable
        QJsonObject loopTool;
        loopTool["type"] = "function";
        QJsonObject loopFunc;
        loopFunc["name"] = "loop_enable";
        loopFunc["description"] = "Turns the continuous AI monitoring loop on or off. Turn off when the objective is complete.";
        QJsonObject loopParams;
        loopParams["type"] = "object";
        QJsonObject loopProps;
        QJsonObject onProp;
        onProp["type"] = "boolean";
        onProp["description"] = "True to enable looping, False to disable.";
        loopProps["on"] = onProp;
        loopParams["properties"] = loopProps;
        QJsonArray loopReq;
        loopReq.append("on");
        loopParams["required"] = loopReq;
        loopFunc["parameters"] = loopParams;
        loopTool["function"] = loopFunc;
        tools.append(loopTool);

        // 15. generate_report
        QJsonObject reportTool;
        reportTool["type"] = "function";
        QJsonObject reportFunc;
        reportFunc["name"] = "generate_report";
        reportFunc["description"] = "Creates a Markdown document combining your text observations and requested saved images.";
        QJsonObject reportParams;
        reportParams["type"] = "object";
        QJsonObject reportProps;
        QJsonObject titleProp;
        titleProp["type"] = "string";
        titleProp["description"] = "Title of the report.";
        reportProps["title"] = titleProp;
        QJsonObject contentProp;
        contentProp["type"] = "string";
        contentProp["description"] = "Markdown-formatted text containing observations and reports.";
        reportProps["content"] = contentProp;
        QJsonObject imagesProp;
        imagesProp["type"] = "array";
        QJsonObject itemsProp;
        itemsProp["type"] = "string";
        imagesProp["items"] = itemsProp;
        imagesProp["description"] = "List of memory keys (from save_image_to_memory) to embed in the report.";
        reportProps["images"] = imagesProp;
        reportParams["properties"] = reportProps;
        QJsonArray reportReq;
        reportReq.append("title");
        reportReq.append("content");
        reportParams["required"] = reportReq;
        reportFunc["parameters"] = reportParams;
        reportTool["function"] = reportFunc;
        tools.append(reportTool);

        // 16. remember — persistent factual memory
        QJsonObject rememberTool;
        rememberTool["type"] = "function";
        QJsonObject rememberFunc;
        rememberFunc["name"] = "remember";
        rememberFunc["description"] = "Store a short factual note in persistent memory (survives restarts). Use this to remember observations, labels, or decisions across the whole session. Returns confirmation.";
        QJsonObject rememberParams;
        rememberParams["type"] = "object";
        QJsonObject rememberProps;
        QJsonObject noteTextProp;
        noteTextProp["type"] = "string";
        noteTextProp["description"] = "The note to remember, e.g. 'The red door leads to the kitchen'.";
        rememberProps["text"] = noteTextProp;
        rememberParams["properties"] = rememberProps;
        QJsonArray rememberReq;
        rememberReq.append("text");
        rememberParams["required"] = rememberReq;
        rememberFunc["parameters"] = rememberParams;
        rememberTool["function"] = rememberFunc;
        tools.append(rememberTool);

        // 17. recall_memory — search persistent memory
        QJsonObject recallMemTool;
        recallMemTool["type"] = "function";
        QJsonObject recallMemFunc;
        recallMemFunc["name"] = "recall_memory";
        recallMemFunc["description"] = "Search persistent memory for notes that match a query (substring match). Use to remember facts/observations from earlier in this or a previous mission.";
        QJsonObject recallMemParams;
        recallMemParams["type"] = "object";
        QJsonObject recallMemProps;
        QJsonObject queryProp;
        queryProp["type"] = "string";
        queryProp["description"] = "Search term(s), e.g. 'door' or 'obstacle'. Leave empty to list recent notes.";
        recallMemProps["query"] = queryProp;
        recallMemParams["properties"] = recallMemProps;
        QJsonArray recallMemReq;
        recallMemReq.append("query");
        recallMemParams["required"] = recallMemReq;
        recallMemFunc["parameters"] = recallMemParams;
        recallMemTool["function"] = recallMemFunc;
        tools.append(recallMemTool);

        return tools;
    }
};
