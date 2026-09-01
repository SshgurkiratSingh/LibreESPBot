#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "network/TelemetryClient.hpp"
#include "network/CommandEmitter.hpp"
#include "network/DiscoveryWorker.hpp"
#include "network/VideoManager.hpp"
#include "mapping/RadarPointCloud.hpp"
#include "core/AppSettings.hpp"
#include "core/ScriptEngine.hpp"
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
    TelemetryClient telemetryClient;
    CommandEmitter commandEmitter;
    DiscoveryWorker discoveryWorker;
    VideoManager videoManager;
    RadarPointCloud radarCloud;
    AppSettings appSettings;
    ScriptEngine scriptEngine(&commandEmitter);
    PanoramaBuilder panoramaBuilder(&commandEmitter, &telemetryClient, &videoManager);
    TurningCalibrator turningCalibrator(&commandEmitter, &telemetryClient);

    // Expose to QML
    engine.rootContext()->setContextProperty("telemetryClient", &telemetryClient);
    engine.rootContext()->setContextProperty("commandEmitter", &commandEmitter);
    engine.rootContext()->setContextProperty("discoveryWorker", &discoveryWorker);
    engine.rootContext()->setContextProperty("videoManager", &videoManager);
    engine.rootContext()->setContextProperty("radarCloud", &radarCloud);
    engine.rootContext()->setContextProperty("appSettings", &appSettings);
    engine.rootContext()->setContextProperty("scriptEngine", &scriptEngine);
    engine.rootContext()->setContextProperty("panoramaBuilder", &panoramaBuilder);
    engine.rootContext()->setContextProperty("turningCalibrator", &turningCalibrator);

    // Start networking layers
    discoveryWorker.startDiscovery();
    telemetryClient.startListening(8889);
    
    // CommandEmitter MUST share the TelemetryClient's bound socket (8889).
    // This punches exactly one bidirectional UDP hole in the stateful firewall,
    // and prevents Linux from dropping packets via dual-socket load balancing.
    commandEmitter.setSharedSocket(telemetryClient.socket());
    
    // Command emitter target is dynamically set upon mDNS discovery, but we can set a default
    commandEmitter.setTargetAddress("192.168.4.1", 8888); 
    
    QObject::connect(&discoveryWorker, &DiscoveryWorker::roverDiscovered,
                     &app, [&commandEmitter](const QString& ip, const QString& profile) {
        qDebug() << "Auto-configuring CommandEmitter to discovered IP:" << ip;
        commandEmitter.setTargetAddress(ip, 8888);
    });
    commandEmitter.startEmitting(20);

    const QUrl url(u"qrc:/RoverControl/qml/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
