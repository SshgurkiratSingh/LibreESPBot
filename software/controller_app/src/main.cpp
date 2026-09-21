#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "network/TelemetryClient.hpp"
#include "network/CommandEmitter.hpp"
#include "network/DiscoveryWorker.hpp"
#include "network/VideoManager.hpp"
#include "network/NodeRegistry.hpp"
#include "network/RoverNode.hpp"
#include "mapping/RadarPointCloud.hpp"
#include "core/Types.hpp"
#include "core/DebugLogger.hpp"
#include "core/AppSettings.hpp"
#include "core/ScriptEngine.hpp"
#include "core/JoystickHandler.hpp"
#include "tools/PanoramaBuilder.hpp"
#include "tools/TurningCalibrator.hpp"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "ai/AiAgentEngine.hpp"
#include "ai/AiToolDispatcher.hpp"
#include "ai/AiImageStore.hpp"
#include "ai/AiMemoryStore.hpp"

#include <QQuickStyle>

#ifdef Q_OS_ANDROID
#include <QtCore/QJniObject>
#include <QtCore/QCoreApplication>
#endif

#include <QThread>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);
    app.setOrganizationName("LibreESP");
    app.setOrganizationDomain("libreesp.org");
    app.setApplicationName("LibreESPBot");

    qInstallMessageHandler(customMessageHandler);

    QQmlApplicationEngine engine;

#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (activity.isValid()) {
            // Force sensor landscape orientation
            activity.callMethod<void>("setRequestedOrientation", "(I)V", 6);
            
            // Allow app to extend into the camera notch (cutout) area
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                window.callMethod<void>("addFlags", "(I)V", 1024); // FLAG_FULLSCREEN
                QJniObject attrs = window.callObjectMethod("getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
                if (attrs.isValid()) {
                    attrs.setField<jint>("layoutInDisplayCutoutMode", 1); // LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
                    window.callMethod<void>("setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V", attrs.object());
                }
            }
            // Acquire MulticastLock to allow mDNS discovery packets to reach the app
            QJniObject wifiManager = activity.callObjectMethod("getSystemService", 
                                       "(Ljava/lang/String;)Ljava/lang/Object;", 
                                       QJniObject::fromString("wifi").object<jstring>());
            if (wifiManager.isValid()) {
                QJniObject lock = wifiManager.callObjectMethod("createMulticastLock", 
                                     "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;", 
                                     QJniObject::fromString("LibreESPBotMulticastLock").object<jstring>());
                if (lock.isValid()) {
                    lock.callMethod<void>("setReferenceCounted", "(Z)V", false);
                    lock.callMethod<void>("acquire");
                    qDebug() << "Android MulticastLock acquired successfully!";
                }
            }
        }
    });
#endif

    // Instantiate backend workers
    AppSettings      appSettings;
    NodeRegistry     nodeRegistry;
    TelemetryClient  telemetryClient(&nodeRegistry);
    CommandEmitter   commandEmitter(&nodeRegistry);
    commandEmitter.setAppSettings(&appSettings);
    DiscoveryWorker  discoveryWorker(&nodeRegistry);
    VideoManager     videoManager;
    RadarPointCloud  radarCloud;
    ScriptEngine     scriptEngine(&commandEmitter, &telemetryClient);
    JoystickHandler  joystickHandler;
    PanoramaBuilder  panoramaBuilder(&commandEmitter, &telemetryClient, &videoManager);
    TurningCalibrator turningCalibrator(&commandEmitter, &telemetryClient);

    AiAgentEngine    aiEngine;
    AiToolDispatcher aiDispatcher;
    AiImageStore     aiImageStore;
    AiMemoryStore    aiMemoryStore;

    // Cross-link telemetry ↔ commandEmitter for PONG tracking
    commandEmitter.setTelemetryClient(&telemetryClient);

    // Expose to QML
    videoManager.setCommandEmitter(&commandEmitter);

    // Link AI Agent to Dispatcher
    QObject::connect(&aiEngine, &AiAgentEngine::toolCallReady, &aiDispatcher, &AiToolDispatcher::dispatchTool);
    
    // Link Dispatcher to Actual Systems
    QObject::connect(&aiDispatcher, &AiToolDispatcher::executeScriptRequested, &scriptEngine, &ScriptEngine::runScript);
    QObject::connect(&aiDispatcher, &AiToolDispatcher::stopRoverRequested, &scriptEngine, &ScriptEngine::stopScript);
    
    QObject::connect(&aiDispatcher, &AiToolDispatcher::saveImageRequested, &aiImageStore, [&videoManager, &aiImageStore](QString label) {
        QByteArray bytes = QByteArray::fromBase64(videoManager.currentFrameBase64().toUtf8());
        QImage img;
        img.loadFromData(bytes);
        aiImageStore.saveImage(img, label);
    });
    QObject::connect(&aiDispatcher, &AiToolDispatcher::saveImageToMemoryRequested, &aiImageStore, [&videoManager, &aiImageStore](QString key) {
        QByteArray bytes = QByteArray::fromBase64(videoManager.currentFrameBase64().toUtf8());
        QImage img;
        img.loadFromData(bytes);
        aiImageStore.saveImageToMemory(img, key);
    });
    // Persistent text memory (remember / recall_memory tools).
    aiEngine.setMemoryStore(&aiMemoryStore);
    QObject::connect(&aiDispatcher, &AiToolDispatcher::rememberRequested, &aiMemoryStore, [&aiMemoryStore](QString text) {
        aiMemoryStore.remember(text);
    });
    QObject::connect(&aiDispatcher, &AiToolDispatcher::recallMemoryRequested, &aiMemoryStore, [&aiMemoryStore](QString query) {
        qDebug() << "[AI] recall_memory query:" << query;
    });
    // describe_view is primarily handled inside AiAgentEngine (it builds the text
    // description + stores it in the image store); this just surfaces the call.
    QObject::connect(&aiDispatcher, &AiToolDispatcher::describeViewRequested, [&](QString key) {
        qDebug() << "[AI] describe_view requested, store key:" << key;
    });
    QObject::connect(&aiDispatcher, &AiToolDispatcher::setHeadlightRequested, &commandEmitter, &CommandEmitter::setHeadlightMode);
    QObject::connect(&aiDispatcher, &AiToolDispatcher::takePanoramaRequested, &panoramaBuilder, &PanoramaBuilder::startPanorama);
    QObject::connect(&aiDispatcher, &AiToolDispatcher::sweepRadarRequested, &commandEmitter, [&commandEmitter](int speed) {
        commandEmitter.setRadarSweepSpeed(speed);
        commandEmitter.setRadarSweep(speed > 0);
    });
    QObject::connect(&aiDispatcher, &AiToolDispatcher::loopEnableRequested, &appSettings, &AppSettings::setAiAutoLoop);
    QObject::connect(&aiDispatcher, &AiToolDispatcher::generateReportRequested, [&](QString title, QString content, QStringList imageKeys) {
        QDir reportDir("reports");
        if (!reportDir.exists()) {
            QDir().mkdir("reports");
        }
        
        QString filename = "reports/" + title.replace(" ", "_").replace(QRegularExpression("[^a-zA-Z0-9_]"), "").toLower() + ".md";
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# " << title << "\n\n";
            out << content << "\n\n";
            
            if (!imageKeys.isEmpty()) out << "## Attached Images\n\n";
            for (const QString& key : imageKeys) {
                QImage img = aiImageStore.getImageFromMemory(key);
                if (!img.isNull()) {
                    QString imgFilename = key + ".png";
                    img.save("reports/" + imgFilename);
                    out << "![" << key << "](" << imgFilename << ")\n\n";
                }
            }
            file.close();
            qDebug() << "Report generated:" << filename;
        }
    });

    // Provide AppSettings to AI Engine
    QObject::connect(&appSettings, &AppSettings::aiApiKeyChanged, [&]() {
        aiEngine.setApiKey(appSettings.aiApiKey());
    });
    QObject::connect(&appSettings, &AppSettings::aiModelNameChanged, [&]() {
        aiEngine.setModelName(appSettings.aiModelName());
    });
    QObject::connect(&appSettings, &AppSettings::aiSystemPromptChanged, [&]() {
        aiEngine.setSystemPrompt(appSettings.aiSystemPrompt());
    });
    QObject::connect(&appSettings, &AppSettings::aiBaseUrlChanged, [&]() {
        aiEngine.setBaseUrl(appSettings.aiBaseUrl());
    });
    QObject::connect(&appSettings, &AppSettings::aiSupportsVisionChanged, [&]() {
        aiEngine.setSupportsVision(appSettings.aiSupportsVision());
    });
    QObject::connect(&appSettings, &AppSettings::aiUserInstructionChanged, [&]() {
        aiEngine.setUserInstruction(appSettings.aiUserInstruction());
    });
    aiEngine.setApiKey(appSettings.aiApiKey());
    aiEngine.setModelName(appSettings.aiModelName());
    aiEngine.setSystemPrompt(appSettings.aiSystemPrompt());
    aiEngine.setBaseUrl(appSettings.aiBaseUrl());
    aiEngine.setSupportsVision(appSettings.aiSupportsVision());
    aiEngine.setUserInstruction(appSettings.aiUserInstruction());

    engine.rootContext()->setContextProperty("telemetryClient", &telemetryClient);
    engine.rootContext()->setContextProperty("commandEmitter",  &commandEmitter);
    engine.rootContext()->setContextProperty("discoveryWorker", &discoveryWorker);
    engine.rootContext()->setContextProperty("videoManager",    &videoManager);
    engine.rootContext()->setContextProperty("radarCloud",      &radarCloud);
    engine.rootContext()->setContextProperty("appSettings",     &appSettings);
    engine.rootContext()->setContextProperty("DebugLogger",     DebugLogger::instance());
    engine.rootContext()->setContextProperty("scriptEngine",    &scriptEngine);
    engine.rootContext()->setContextProperty("joystickHandler", &joystickHandler);
    engine.rootContext()->setContextProperty("panoramaBuilder", &panoramaBuilder);
    engine.rootContext()->setContextProperty("turningCalibrator", &turningCalibrator);
    engine.rootContext()->setContextProperty("nodeRegistry",    &nodeRegistry);
    
    aiEngine.setImageStore(&aiImageStore);
    aiEngine.setFrameProvider([&videoManager]() {
        QImage img;
        QString b64 = videoManager.currentFrameBase64();
        if (!b64.isEmpty()) {
            img.loadFromData(QByteArray::fromBase64(b64.toUtf8()));
        }
        return img;
    });
    engine.rootContext()->setContextProperty("AiEngine", &aiEngine);
    engine.rootContext()->setContextProperty("AiImageStoreModel", &aiImageStore);
    engine.rootContext()->setContextProperty("AiMemoryStoreModel", &aiMemoryStore);

    // Start networking layers
    discoveryWorker.startDiscovery();
    telemetryClient.startListening(LBP_PORT_TEL);

    // Diagnostic: print expected struct sizes so we can verify firmware/app agreement
    qDebug() << "[DIAG] sizeof(VehicleTelemetryPacket) =" << sizeof(VehicleTelemetryPacket);
    qDebug() << "[DIAG] sizeof(VehicleCommandPacket)   =" << sizeof(VehicleCommandPacket);
    qDebug() << "[DIAG] sizeof(LbpBeaconPacket)        =" << sizeof(LbpBeaconPacket);
    qDebug() << "[DIAG] LBP_PROTOCOL_VERSION           =" << static_cast<int>(LBP_PROTOCOL_VERSION);

    // CommandEmitter MUST share the TelemetryClient's bound socket.
    // This punches one bidirectional UDP hole in the stateful firewall.
    commandEmitter.setSharedSocket(telemetryClient.socket());

    // Auto-configure CommandEmitter when user selects a node, or when the
    // first node is auto-selected by NodeRegistry.
    QObject::connect(&nodeRegistry, &NodeRegistry::activeNodeChanged,
                     &app, [&commandEmitter](RoverNode* node) {
        if (node) {
            qDebug() << "[main] Active node changed to" << node->ip() << "-" << node->boardName();
            commandEmitter.setTargetAddress(node->ip(), LBP_PORT_CMD);
            commandEmitter.startEmitting(20);
        } else {
            commandEmitter.stopEmitting();
        }
    });

    // Bind video manager FPS to settings
    videoManager.setTargetFps(appSettings.cameraFps());
    QObject::connect(&appSettings, &AppSettings::cameraFpsChanged, [&](){
        videoManager.setTargetFps(appSettings.cameraFps());
    });

    const QUrl url(u"qrc:/RoverControl/qml/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
