#include "AiToolDispatcher.hpp"
#include <QDebug>
#include <QJsonArray>

AiToolDispatcher::AiToolDispatcher(QObject* parent) : QObject(parent) {
}

void AiToolDispatcher::dispatchTool(const QString& name, const QJsonObject& args) {
    qDebug() << "AiToolDispatcher executing tool:" << name << "with args:" << args;

    if (name == "run_script") {
        emit executeScriptRequested(args["script"].toString());
    } else if (name == "stop_rover") {
        emit stopRoverRequested();
    } else if (name == "save_image") {
        emit saveImageRequested(args["label"].toString());
    } else if (name == "save_image_to_memory") {
        emit saveImageToMemoryRequested(args["key"].toString());
    } else if (name == "recall_image") {
        emit recallImageRequested(args["key"].toString());
    } else if (name == "describe_view") {
        emit describeViewRequested(args["key"].toString());
    } else if (name == "set_headlight") {
        emit setHeadlightRequested(args["mode"].toInt());
    } else if (name == "set_servo") {
        emit setServoRequested(args["angle"].toInt());
    } else if (name == "sweep_radar") {
        emit sweepRadarRequested(args["speed"].toInt());
    } else if (name == "wait_for_telemetry") {
        emit waitForTelemetryRequested(args["condition"].toString(), args["timeout_ms"].toInt());
    } else if (name == "speak") {
        emit speakRequested(args["text"].toString());
    } else if (name == "log_note") {
        emit logNoteRequested(args["text"].toString());
    } else if (name == "take_panorama") {
        emit takePanoramaRequested();
    } else if (name == "ask_user") {
        emit askUserRequested(args["question"].toString());
    } else if (name == "loop_enable") {
        emit loopEnableRequested(args["on"].toBool());
    } else if (name == "remember") {
        emit rememberRequested(args["text"].toString());
    } else if (name == "recall_memory") {
        emit recallMemoryRequested(args["query"].toString());
    } else if (name == "generate_report") {
        QString title = args["title"].toString();
        QString content = args["content"].toString();
        QStringList imageKeys;
        if (args.contains("images")) {
            QJsonArray arr = args["images"].toArray();
            for (int i = 0; i < arr.size(); ++i) {
                imageKeys.append(arr[i].toString());
            }
        }
        emit generateReportRequested(title, content, imageKeys);
    } else {
        qWarning() << "AiToolDispatcher: Unknown tool call requested:" << name;
    }
}
