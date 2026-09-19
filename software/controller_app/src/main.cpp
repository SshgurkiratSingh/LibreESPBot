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
    NodeRegistry     nodeRegistry;
    TelemetryClient  telemetryClient(&nodeRegistry);
    CommandEmitter   commandEmitter(&nodeRegistry);
    DiscoveryWorker  discoveryWorker(&nodeRegistry);
    VideoManager     videoManager;
    RadarPointCloud  radarCloud;
    AppSettings      appSettings;
    ScriptEngine     scriptEngine(&commandEmitter, &telemetryClient);
    JoystickHandler  joystickHandler;
    PanoramaBuilder  panoramaBuilder(&commandEmitter, &telemetryClient, &videoManager);
    TurningCalibrator turningCalibrator(&commandEmitter, &telemetryClient);

    // Cross-link telemetry ↔ commandEmitter for PONG tracking
    commandEmitter.setTelemetryClient(&telemetryClient);

    // Expose to QML
    videoManager.setCommandEmitter(&commandEmitter);

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
