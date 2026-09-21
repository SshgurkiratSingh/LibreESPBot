#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QAbstractListModel>
#include <functional>

class AiImageStore;
class AiMemoryStore;

class AiAgentEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(bool supportsVision READ supportsVision NOTIFY supportsVisionChanged)
    Q_PROPERTY(QString thinkingLog READ thinkingLog NOTIFY thinkingLogChanged)
    Q_PROPERTY(QString responseLog READ responseLog NOTIFY responseLogChanged)
    Q_PROPERTY(QString errorLog READ errorLog NOTIFY errorLogChanged)
    Q_PROPERTY(QString pastThinkingLog READ pastThinkingLog NOTIFY pastThinkingChanged)
    Q_PROPERTY(QObject* toolLogModel READ toolLogModel CONSTANT)
public:
    explicit AiAgentEngine(QObject* parent = nullptr);
    ~AiAgentEngine();

    void setImageStore(AiImageStore* store) { m_imageStore = store; }
    void setMemoryStore(AiMemoryStore* store) { m_memoryStore = store; }
    void setUserInstruction(const QString& instruction) { m_userInstruction = instruction; }

    // Some providers (and text-only models like DeepSeek-v4-pro) reject image_url
    // content with HTTP 400. We therefore expose per-model vision detection + an
    // override, and use local OpenCV scene descriptions as a text-based sight
    // channel for text-only models (vision-capable models get real base64 pixels).
    void setApiKey(const QString& apiKey);
    void setModelName(const QString& model);
    void setSystemPrompt(const QString& prompt);
    void setBaseUrl(const QString& baseUrl);
    void setSupportsVision(bool override);
    void setFrameProvider(const std::function<QImage()>& provider);
    void captureContext();
    void buildMessages();
    void sendRequest();

    bool isRunning() const { return m_isRunning; }
    bool supportsVision() const;
    QString thinkingLog() const { return m_thinkingLog; }
    QString responseLog() const { return m_responseLog; }
    QString errorLog() const { return m_errorLog; }
    QString pastThinkingLog() const;
    QObject* toolLogModel() const { return m_toolLogModel; }

    Q_INVOKABLE void startMission(const QString& task);
    Q_INVOKABLE void stopMission();
    Q_INVOKABLE void provideUserAnswer(const QString& toolCallId, const QString& answer);

    void addToolLog(const QString& time, const QString& name, const QString& status, const QString& args);

signals:
    void thinkingToken(QString token);
    void toolCallReady(QString name, QJsonObject args);
    void responseToken(QString token);
    void finished();
    void errorOccurred(QString errorString);
    void userAnswerRequested(QString question, QString toolCallId);
    void visionNotice(QString text);

    void isRunningChanged();
    void supportsVisionChanged();
    void thinkingLogChanged();
    void responseLogChanged();
    void errorLogChanged();
    void pastThinkingChanged();

private slots:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError code);

private:
    // True when the selected model can ingest image_url content parts.
    bool modelSupportsVision(const QString& model) const;
        // Build the `content` (text) value of a tool result message for one tool call.
    // Vision-capable models additionally have their images collected into `images`
    // (base64) / `captions` so they can be attached in a *user* message afterwards
    // (DeepSeek allows images in user messages only).
    QString buildToolResultContent(const QString& toolName, const QJsonObject& args,
                                   QList<QString>& images, QList<QString>& captions);
    // Append a single user message carrying the collected tool-result images after
    // the tool results, so images appear in a legal (user) role.
    void appendVisionMessage(const QList<QString>& images, const QList<QString>& captions);
    // Text analysis of the current camera frame (returns a diagnostics string if
    // there is no usable frame, so tool results always carry evidence).
    QString sceneDescription();
    // Append a fresh camera/sensor scene description so the model "sees" the world.
    void appendSceneContext();

    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_reply;
    QString m_apiKey;
    QString m_modelName = QStringLiteral("deepseek-chat");
    QString m_baseUrl = QStringLiteral("https://api.deepseek.com");
    QString m_systemPrompt = QStringLiteral("You are the autonomous AI agent for an ESP32-CAM rover. Reason carefully before acting.");
    QString m_currentTask;
    QString m_buffer;
    bool m_forceVision = false;
    bool m_inThinkBlock = false;
    std::function<QImage()> m_frameProvider;
    AiMemoryStore* m_memoryStore = nullptr;
    QString m_userInstruction;

    QMap<int, QString> m_toolCallNames;
    QMap<int, QString> m_toolCallArgs;
    QMap<int, QString> m_toolCallIds;
    QJsonArray m_messageHistory;

    bool m_isRunning = false;
    QString m_thinkingLog;
    QString m_responseLog;
    QString m_errorLog;
    QStringList m_pastThinking;
    QObject* m_toolLogModel = nullptr;
    AiImageStore* m_imageStore = nullptr;
};

