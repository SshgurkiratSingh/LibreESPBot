#include "AiAgentEngine.hpp"
#include "AiToolSpec.hpp"
#include "AiImageStore.hpp"
#include "AiMemoryStore.hpp"
#include "AiVision.hpp"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDateTime>
#include <QAbstractListModel>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QStringList>
#include <QDebug>

class ToolLogModel : public QAbstractListModel {
public:
    enum Roles { TimeRole = Qt::UserRole + 1, NameRole, StatusRole, ArgsRole };
    struct LogEntry { QString time; QString name; QString status; QString args; };
    QList<LogEntry> logs;

    ToolLogModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return logs.size();
    }
    
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= logs.size()) return QVariant();
        const auto& item = logs[index.row()];
        switch (role) {
            case TimeRole: return item.time;
            case NameRole: return item.name;
            case StatusRole: return item.status;
            case ArgsRole: return item.args;
        }
        return QVariant();
    }
    
    QHash<int, QByteArray> roleNames() const override {
        return {{TimeRole, "time"}, {NameRole, "toolName"}, {StatusRole, "status"}, {ArgsRole, "argsText"}};
    }
    
    void addLog(const QString& t, const QString& n, const QString& s, const QString& a) {
        beginInsertRows(QModelIndex(), 0, 0); // Insert at top
        logs.prepend({t, n, s, a});
        endInsertRows();
    }
};

AiAgentEngine::AiAgentEngine(QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_reply(nullptr), m_inThinkBlock(false) {
    m_toolLogModel = new ToolLogModel(this);
    
    connect(this, &AiAgentEngine::thinkingToken, this, [this](QString t) {
        m_thinkingLog += t;
        emit thinkingLogChanged();
    });
    connect(this, &AiAgentEngine::responseToken, this, [this](QString t) {
        m_responseLog += t;
        emit responseLogChanged();
    });
    connect(this, &AiAgentEngine::errorOccurred, this, [this](QString err) {
        m_errorLog += QStringLiteral("[ERROR] ") + err + "\n";
        m_isRunning = false;
        emit isRunningChanged();
        emit errorLogChanged();
    });
}

AiAgentEngine::~AiAgentEngine() {
}

void AiAgentEngine::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey;
}

void AiAgentEngine::setModelName(const QString& model) {
    if (!model.isEmpty() && m_modelName != model) {
        m_modelName = model;
        emit supportsVisionChanged(); // computed vision capability may change
    }
}

void AiAgentEngine::setBaseUrl(const QString& baseUrl) {
    m_baseUrl = baseUrl.trimmed();
    if (m_baseUrl.isEmpty()) m_baseUrl = QStringLiteral("https://api.deepseek.com");
}

void AiAgentEngine::setSupportsVision(bool override) {
    if (m_forceVision != override) {
        m_forceVision = override;
        emit supportsVisionChanged();
    }
}

void AiAgentEngine::setFrameProvider(const std::function<QImage()>& provider) {
    m_frameProvider = provider;
}

bool AiAgentEngine::supportsVision() const {
    return m_forceVision || modelSupportsVision(m_modelName);
}

void AiAgentEngine::setSystemPrompt(const QString& prompt) {
    if (!prompt.isEmpty()) m_systemPrompt = prompt;
}

bool AiAgentEngine::modelSupportsVision(const QString& model) const {
    QString m = model.trimmed().toLower();
    if (m.isEmpty()) return false;
    if (m.contains("deepseek")) {
        // Per DeepSeek Vision docs: deepseek-flash (DeepSeek-V4.1-Flash) accepts
        // images; the legacy alias deepseek-v4-flash-vision-exp is also served by
        // Flash. deepseek-v4-pro, deepseek-chat and deepseek-reasoner are text-only.
        return m.contains("flash")
            && !m.contains("pro")
            && !m.contains("chat")
            && !m.contains("reasoner");
    }
    // Known multimodal families (OpenAI-compatible gateways, etc.)
    if (m.contains("gpt-4o") || m.contains("gpt-4.1") || m.contains("gpt-5") ||
        m.contains("vision") || m.contains("vl") || m.contains("-vlm") ||
        m.contains("gemini") || m.contains("llava") || m.contains("internvl") ||
        m.contains("minicpm-v") || m.contains("pixtral")) return true;
    return false;
}

void AiAgentEngine::startMission(const QString& task) {
    if (m_isRunning) return;
    m_isRunning = true;
    m_currentTask = task;
    
    m_messageHistory = QJsonArray();
    // Inject the persisted user instruction into the system prompt so it applies
    // to the whole mission.
    QString systemContent = m_systemPrompt;
    QString userInstr = m_userInstruction.trimmed();
    if (!userInstr.isEmpty()) {
        systemContent += QStringLiteral("\n\n[USER INSTRUCTIONS - follow these on every mission]\n%1").arg(userInstr);
    }
    // Inject the most recent persistent memory notes so the agent starts each
    // mission already aware of facts saved in previous missions. Full recall
    // remains available via the recall_memory tool.
    if (m_memoryStore) {
        QStringList notes = m_memoryStore->allNotes();
        if (!notes.isEmpty()) {
            QStringList top;
            for (int i = 0; i < notes.size() && i < 12; ++i) top << QStringLiteral("- ") + notes[i];
            systemContent += QStringLiteral("\n\n[PERSISTENT MEMORY - facts saved in previous missions, newest first]\n%1")
                                 .arg(top.join("\n"));
        }
    }
    m_messageHistory.append(QJsonObject{{"role", "system"}, {"content", systemContent}});

    // Seed the mission with a live text description of the current view so the
    // model (even a text-only DeepSeek) starts with real "sight".
    QString initial = m_currentTask.trimmed();
    if (m_frameProvider) {
        QImage frame;
        bool haveFrame = false;
        try { frame = m_frameProvider(); haveFrame = !frame.isNull(); } catch (...) {}
        QString scene = haveFrame ? AiVision::describe(frame)
                                  : QStringLiteral("<no camera frame available>");
        if (m_imageStore) m_imageStore->setMemoryDescription("current_view", scene);
        if (!initial.isEmpty()) initial += QStringLiteral("\n\n[LIVE CAMERA]\n%1").arg(scene);
        emit visionNotice(scene);
    }
    if (!initial.isEmpty()) {
        m_messageHistory.append(QJsonObject{{"role", "user"}, {"content", initial}});
    }

    emit isRunningChanged();
    m_thinkingLog.clear();
    m_responseLog.clear();
    m_errorLog.clear();
    emit thinkingLogChanged();
    emit responseLogChanged();
    emit errorLogChanged();
    sendRequest();
}

void AiAgentEngine::provideUserAnswer(const QString& toolCallId, const QString& answer) {
    if (!m_isRunning) return;
    
    QJsonObject toolResultMsg;
    toolResultMsg["role"] = "tool";
    toolResultMsg["tool_call_id"] = toolCallId;
    
    QJsonObject ansObj;
    ansObj["status"] = "success";
    ansObj["answer"] = answer;
    toolResultMsg["content"] = QString::fromUtf8(QJsonDocument(ansObj).toJson(QJsonDocument::Compact));
    
    m_messageHistory.append(toolResultMsg);
    
    addToolLog(QDateTime::currentDateTime().toString("HH:mm:ss"), "ask_user_response", "OK", answer);
    
    sendRequest();
}

void AiAgentEngine::stopMission() {
    if (!m_isRunning) return;
    m_isRunning = false;
    emit isRunningChanged();
    if (m_reply) {
        m_reply->abort();
    }
}

void AiAgentEngine::addToolLog(const QString& time, const QString& name, const QString& status, const QString& args) {
    static_cast<ToolLogModel*>(m_toolLogModel)->addLog(time, name, status, args);
}

void AiAgentEngine::captureContext() {
    // Feed the model a live text description of what the camera currently sees.
    appendSceneContext();
}

void AiAgentEngine::buildMessages() {
    // Messages are assembled incrementally by sendRequest/onFinished; nothing
    // extra to pre-build here.
    qDebug() << "AiAgentEngine: Messages built (incremental assembly active).";
}

void AiAgentEngine::sendRequest() {
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (m_apiKey.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("API key is empty. Please set your DeepSeek API key in the AI Agent panel."));
        return;
    }

    QString endpoint = m_baseUrl;
    if (!endpoint.endsWith(QStringLiteral("/chat/completions"))) {
        endpoint += endpoint.endsWith(QStringLiteral("/")) ? QStringLiteral("chat/completions")
                                                           : QStringLiteral("/chat/completions");
    }
    QNetworkRequest request{QUrl(endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey.trimmed()).toUtf8());

    QJsonObject payload;
    payload["model"] = m_modelName;
    payload["stream"] = true;
    payload["messages"] = m_messageHistory;
    payload["tools"] = AiToolSpec::getToolDefinitions();

    QJsonDocument doc(payload);
    m_reply = m_networkManager->post(request, doc.toJson());

    connect(m_reply, &QNetworkReply::readyRead, this, &AiAgentEngine::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &AiAgentEngine::onFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_reply, &QNetworkReply::errorOccurred, this, &AiAgentEngine::onError);
#else
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &AiAgentEngine::onError);
#endif

    m_inThinkBlock = false;
    m_buffer.clear();
    m_toolCallNames.clear();
    m_toolCallArgs.clear();
    m_toolCallIds.clear();
}

void AiAgentEngine::onReadyRead() {
    if (!m_reply) return;

    m_buffer += QString::fromUtf8(m_reply->readAll());

    // SSE may deliver complete lines; keep a trailing partial line buffered.
    QStringList lines = m_buffer.split("\n");
    m_buffer = lines.takeLast();

    for (const QString& line : lines) {
        QString l = line.trimmed();
        if (l.isEmpty()) continue;
        if (!l.startsWith("data:")) continue;
        QString data = l.mid(5).trimmed();
        if (data == "[DONE]") continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            // Some streams emit "data: {json}\n\ndata: ..." — tolerate stray tokens.
            continue;
        }

        QJsonArray choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) continue;

        QJsonObject choice = choices.first().toObject();
        if (choice.contains("finish_reason") && choice["finish_reason"].toString() == "tool_calls") {
            continue; // streaming is complete; onFinished() drives the tool loop
        }

        QJsonObject delta = choice["delta"].toObject();
        if (delta.contains("reasoning_content")) {
            QString r = delta["reasoning_content"].toString();
            if (!r.isEmpty()) emit thinkingToken(r);
        }
        if (delta.contains("content")) {
            QString c = delta["content"].toString();
            if (!c.isEmpty()) emit responseToken(c);
        }
        if (delta.contains("tool_calls")) {
            QJsonArray toolCalls = delta["tool_calls"].toArray();
            for (const QJsonValue& val : toolCalls) {
                QJsonObject tc = val.toObject();
                QJsonObject func = tc["function"].toObject();
                int index = tc.contains("index") ? tc["index"].toInt() : 0;

                if (tc.contains("id"))      m_toolCallIds[index] = tc["id"].toString();
                if (func.contains("name")) {
                    m_toolCallNames[index] = func["name"].toString();
                    m_toolCallArgs[index] = "";
                }
                if (func.contains("arguments")) m_toolCallArgs[index] += func["arguments"].toString();
            }
        }
    }
}

void AiAgentEngine::onFinished() {
    if (!m_reply) return;

    if (!m_toolCallNames.isEmpty()) {
        // Ordered tool-call list (DeepSeek streams contiguous indexes 0..n-1).
        // QMap is key-ordered, so keys() is already in ascending call index order.
        QList<int> order = m_toolCallNames.keys();

        // 1) Assistant message carrying the tool_calls (must precede results).
        QJsonObject assistantMsg;
        assistantMsg["role"] = "assistant";
        if (!m_thinkingLog.isEmpty()) assistantMsg["reasoning_content"] = m_thinkingLog;
        if (!m_responseLog.isEmpty()) assistantMsg["content"] = m_responseLog;

        QJsonArray toolCallsArray;
        for (const int& index : order) {
            QJsonObject tcObj;
            tcObj["id"]   = m_toolCallIds.value(index);
            tcObj["type"] = "function";
            QJsonObject funcObj;
            funcObj["name"]      = m_toolCallNames.value(index);
            funcObj["arguments"] = m_toolCallArgs.value(index);
            tcObj["function"] = funcObj;
            toolCallsArray.insert(toolCallsArray.size(), tcObj);
        }
        assistantMsg["tool_calls"] = toolCallsArray;
        m_messageHistory.append(assistantMsg);

        // 2) Dispatch tools, capture their results, and append tool results
        //    strictly after the assistant tool_calls message. Images produced by
        //    vision tools are collected and attached later in a USER message.
        QList<QJsonObject> results;
        QList<QString> pendingImages;
        QList<QString> pendingCaptions;
        QString pendingAskUserId;
        QString pendingAskUserQuestion;
        for (const int& index : order) {
            QString name    = m_toolCallNames.value(index);
            QString argsStr = m_toolCallArgs.value(index);
            QString id      = m_toolCallIds.value(index);

            QJsonDocument doc = QJsonDocument::fromJson(argsStr.toUtf8());
            QJsonObject args = doc.object();

            emit toolCallReady(name, args);

            QString resultContent = buildToolResultContent(name, args, pendingImages, pendingCaptions);
            QJsonObject toolResultMsg;
            toolResultMsg["role"] = "tool";
            toolResultMsg["tool_call_id"] = id;
            toolResultMsg["content"] = resultContent;
            results.append(toolResultMsg);

            QString argsLine = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
            if (!resultContent.startsWith(QStringLiteral("{\"status\""))) {
                argsLine += QStringLiteral("\n→ ") + resultContent;
            }
            addToolLog(QDateTime::currentDateTime().toString("HH:mm:ss"), name, "OK", argsLine);

            if (name == "ask_user" && args.contains("question")) {
                pendingAskUserId      = id;
                pendingAskUserQuestion = args["question"].toString();
            }
        }
        for (const QJsonObject& r : results) m_messageHistory.append(r);

        m_toolCallNames.clear();
        m_toolCallArgs.clear();
        m_toolCallIds.clear();

        // Archive this round's reasoning into past-thinking history (newest first).
        if (!m_thinkingLog.trimmed().isEmpty()) {
            m_pastThinking.prepend(m_thinkingLog.trimmed());
            while (m_pastThinking.size() > 20) m_pastThinking.removeAt(m_pastThinking.size() - 1);
            emit pastThinkingChanged();
        }

        m_thinkingLog.clear();
        m_responseLog.clear();
        emit thinkingLogChanged();
        emit responseLogChanged();

        m_reply->deleteLater();
        m_reply = nullptr;

        if (!pendingAskUserId.isEmpty()) {
            emit userAnswerRequested(pendingAskUserQuestion, pendingAskUserId);
        } else {
            // Images belong in a user message (DeepSeek rejects images in
            // assistant/system). Attach them before the next model turn.
            appendVisionMessage(pendingImages, pendingCaptions);
            appendSceneContext();
            sendRequest();
        }
        return;
    }

    // Final answer with no further tool calls: archive any trailing reasoning.
    if (!m_thinkingLog.trimmed().isEmpty()) {
        m_pastThinking.prepend(m_thinkingLog.trimmed());
        while (m_pastThinking.size() > 20) m_pastThinking.removeAt(m_pastThinking.size() - 1);
        emit pastThinkingChanged();
    }

    if (m_isRunning) emit finished();

    m_reply->deleteLater();
    m_reply = nullptr;
}


static QString imageToBase64Jpeg(const QImage& img) {
    if (img.isNull()) return QString();
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    if (!img.save(&buffer, "JPG", 85)) return QString();
    return QString(ba.toBase64());
}

void AiAgentEngine::appendSceneContext() {
    if (!m_isRunning) return;

    // Don't stack two consecutive user messages (breaks some backends).
    if (!m_messageHistory.isEmpty() &&
        m_messageHistory.last().toObject().value("role").toString() == "user") {
        return;
    }

    QImage frame;
    bool haveFrame = false;
    if (m_frameProvider) {
        try { frame = m_frameProvider(); haveFrame = !frame.isNull(); } catch (...) {}
    }
    QString scene = haveFrame ? AiVision::describe(frame)
                              : QStringLiteral("<no live camera feed>");
    if (m_imageStore) m_imageStore->setMemoryDescription("current_view", scene);

    m_messageHistory.append(QJsonObject{{"role", "user"},
                                          {"content", QStringLiteral("[LIVE CAMERA]\n%1").arg(scene)}});
    emit visionNotice(scene);
}

QString AiAgentEngine::pastThinkingLog() const {
    if (m_pastThinking.isEmpty()) return QString();
    return m_pastThinking.join(QStringLiteral("\n\n-- past round --\n\n"));
}


QString AiAgentEngine::sceneDescription() {
    if (!m_frameProvider) return QStringLiteral("<no camera frame available>");
    try {
        QImage frame = m_frameProvider();
        if (frame.isNull()) return QStringLiteral("<no camera frame available>");
        return AiVision::describe(frame);
    } catch (...) {
        return QStringLiteral("<camera analysis error>");
    }
}


QString AiAgentEngine::buildToolResultContent(const QString& toolName, const QJsonObject& args,
                                              QList<QString>& images, QList<QString>& captions) {
    const bool viz = supportsVision();
    QString desc;

    if (toolName == "describe_view") {
        QImage frame;
        bool haveFrame = false;
        if (m_frameProvider) {
            try { frame = m_frameProvider(); haveFrame = !frame.isNull(); } catch (...) {}
        }
        desc = haveFrame ? AiVision::describe(frame)
                         : QStringLiteral("<no camera frame available to describe>");
        QString key = args["key"].toString();
        if (m_imageStore) {
            m_imageStore->setMemoryDescription("current_view", desc);
            if (!key.isEmpty()) m_imageStore->setMemoryDescription(key, desc);
        }
        if (viz && haveFrame) {
            images.append(imageToBase64Jpeg(frame));
            captions.append(key.isEmpty() ? QStringLiteral("current live view")
                                          : QStringLiteral("describe_view '%1'").arg(key));
        }
        return QStringLiteral("[CURRENT VIEW]\n%1").arg(desc);
    }

    if (toolName == "recall_image") {
        QString key = args["key"].toString();
        desc = m_imageStore ? m_imageStore->recallDescription(key)
                            : QStringLiteral("<no image store>");
        if (viz && m_imageStore) {
            QString raw = m_imageStore->recallImageAsBase64(key);
            if (!raw.isEmpty()) {
                images.append(raw);
                captions.append(QStringLiteral("recalled memory image '%1'").arg(key));
            }
        }
        return QStringLiteral("[RECALLED '%1']\n%2").arg(key).arg(desc);
    }

    if (toolName == "save_image" || toolName == "save_image_to_memory") {
        QString key = (toolName == "save_image") ? args["label"].toString()
                                                 : args["key"].toString();
        QImage frame;
        bool haveFrame = false;
        if (m_frameProvider) {
            try { frame = m_frameProvider(); haveFrame = !frame.isNull(); } catch (...) {}
        }
        QString scene = haveFrame ? AiVision::describe(frame)
                                  : QStringLiteral("<no camera frame available>");

        if (toolName == "save_image") {
            desc = QStringLiteral("Saved current camera frame to disk as '%1'.\n%2")
                       .arg(key).arg(scene);
            if (viz && haveFrame) {
                images.append(imageToBase64Jpeg(frame));
                captions.append(QStringLiteral("saved disk image '%1'").arg(key));
            }
        } else {
            // The dispatcher already stored the image + its description; prefer
            // the authoritative stored description so the result matches memory.
            QString stored = m_imageStore ? m_imageStore->recallDescription(key) : QString();
            if (!stored.isEmpty() && !stored.startsWith("<no stored")) {
                desc = QStringLiteral("Stored camera frame under key '%1'.\n%2")
                           .arg(key).arg(stored);
            } else {
                desc = QStringLiteral("Stored camera frame under key '%1'.\n%2")
                           .arg(key).arg(scene);
            }
            if (viz && m_imageStore) {
                QString raw = m_imageStore->recallImageAsBase64(key);
                if (!raw.isEmpty()) {
                    images.append(raw);
                    captions.append(QStringLiteral("stored memory image '%1'").arg(key));
                }
            }
        }
        return desc;
    }

    // Persistent text memory: record a fact/note and search past ones.
    if (toolName == "remember") {
        QString text = args["text"].toString();
        return m_memoryStore ? m_memoryStore->remember(text)
                             : QStringLiteral("<no memory store>");
    }

    if (toolName == "recall_memory") {
        QString query = args["query"].toString();
        QStringList results = m_memoryStore ? m_memoryStore->recall(query) : QStringList();
        if (results.isEmpty()) return QStringLiteral("[MEMORY] No matching notes found.");
        return QStringLiteral("[MEMORY recall '%1']\n%2").arg(query, results.join(QStringLiteral("\n")));
    }


    // Ordinary tools just report success as plain text (safe for text-only APIs).
    return QStringLiteral("{\"status\":\"success\"}");
}

void AiAgentEngine::appendVisionMessage(const QList<QString>& images, const QList<QString>& captions) {
    if (images.isEmpty()) return;

    QJsonArray parts;
    QJsonObject textPart;
    textPart["type"] = "text";
    textPart["text"] = QStringLiteral("[Camera image(s) attached from your tool calls]\n%1")
                            .arg(captions.isEmpty() ? QStringLiteral("images above") : captions.join("\n"));
    parts.append(textPart);

    for (const QString& b64 : images) {
        QJsonObject imgPart;
        imgPart["type"] = "image_url";
        QJsonObject urlObj;
        urlObj["url"] = QStringLiteral("data:image/jpeg;base64,%1").arg(b64);
        imgPart["image_url"] = urlObj;
        parts.append(imgPart);
    }

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = parts;
    m_messageHistory.append(userMsg);
}

void AiAgentEngine::onError(QNetworkReply::NetworkError code) {
    QString errStr;
    if (m_reply) {
        errStr = m_reply->errorString();
        // Also try to read the body for API error messages
        QByteArray body = m_reply->readAll();
        if (!body.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject() && doc.object().contains("error")) {
                errStr += " — " + doc.object()["error"].toObject()["message"].toString();
            }
        }
    } else {
        errStr = QStringLiteral("Unknown network error (code %1)").arg(static_cast<int>(code));
    }
    qWarning() << "AiAgentEngine Network Error:" << errStr;
    emit errorOccurred(errStr);
}
