#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "network/TelemetryClient.hpp"
#include "network/CommandEmitter.hpp"
#include "network/DiscoveryWorker.hpp"
#include "mapping/RadarPointCloud.hpp"

#include <QQuickStyle>

#ifdef Q_OS_ANDROID
#include <QtCore/QJniObject>
#include <QtCore/QCoreApplication>
#endif

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

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
        }
    });
#endif

    // Instantiate backend workers
    TelemetryClient telemetryClient;
    CommandEmitter commandEmitter;
    DiscoveryWorker discoveryWorker;
    RadarPointCloud radarCloud;

    // Expose to QML
    engine.rootContext()->setContextProperty("telemetryClient", &telemetryClient);
    engine.rootContext()->setContextProperty("commandEmitter", &commandEmitter);
    engine.rootContext()->setContextProperty("discoveryWorker", &discoveryWorker);
    engine.rootContext()->setContextProperty("radarCloud", &radarCloud);

    // Start networking layers
    discoveryWorker.startDiscovery();
    telemetryClient.startListening(8889);
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
