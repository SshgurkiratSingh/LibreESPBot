#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "network/TelemetryClient.hpp"
#include "network/CommandEmitter.hpp"
#include "network/DiscoveryWorker.hpp"
#include "mapping/RadarPointCloud.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

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
    commandEmitter.startEmitting(20);

    const QUrl url(u"qrc:/qt/qml/RoverControl/qml/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
