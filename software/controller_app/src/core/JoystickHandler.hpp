#pragma once

#include <QObject>
#include <QThread>
#include <QVariantMap>
#include <QString>
#include <QSettings>

class JoystickWorker;

class JoystickHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QVariantMap mapping READ mapping WRITE setMapping NOTIFY mappingChanged)
    Q_PROPERTY(bool isLearning READ isLearning WRITE setIsLearning NOTIFY isLearningChanged)
    Q_PROPERTY(QString lastLearnedInput READ lastLearnedInput NOTIFY lastLearnedInputChanged)

public:
    explicit JoystickHandler(QObject *parent = nullptr);
    ~JoystickHandler();

    bool connected() const { return m_connected; }
    QString deviceName() const { return m_deviceName; }
    
    QVariantMap mapping() const { return m_mapping; }
    void setMapping(const QVariantMap& map);

    bool isLearning() const { return m_isLearning; }
    void setIsLearning(bool learning);

    QString lastLearnedInput() const { return m_lastLearnedInput; }

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void saveMapping();

signals:
    void connectedChanged();
    void deviceNameChanged();
    void mappingChanged();
    void isLearningChanged();
    void lastLearnedInputChanged();

    // Mapped logical signals
    void mappedAxisChanged(const QString& actionName, float value);
    void mappedButtonChanged(const QString& actionName, bool pressed);

    // Raw signals for debugging or learning
    void rawAxisChanged(int axis, float value);
    void rawButtonChanged(int button, bool pressed);

private slots:
    void onWorkerConnected(const QString& name);
    void onWorkerDisconnected();
    void onRawAxis(int axis, float value);
    void onRawButton(int button, bool pressed);

private:
    void loadMapping();
    
    JoystickWorker* m_worker;
    QThread* m_thread;
    
    bool m_connected;
    QString m_deviceName;
    QVariantMap m_mapping;
    bool m_isLearning;
    QString m_lastLearnedInput;
};
