#include "JoystickHandler.hpp"
#include <QDebug>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFile>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#endif

class JoystickWorker : public QObject {
    Q_OBJECT
public:
    explicit JoystickWorker(QObject *parent = nullptr) : QObject(parent), m_running(false), m_fd(-1) {}
    ~JoystickWorker() { stop(); }

public slots:
    void start() {
        m_running = true;
#ifdef Q_OS_LINUX
        while (m_running) {
            if (m_fd < 0) {
                // Try to find a joystick
                for (int i = 0; i < 4; ++i) {
                    QString path = QString("/dev/input/js%1").arg(i);
                    if (QFile::exists(path)) {
                        m_fd = ::open(path.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
                        if (m_fd >= 0) {
                            char name[128];
                            if (ioctl(m_fd, JSIOCGNAME(sizeof(name)), name) < 0)
                                strncpy(name, "Unknown", sizeof(name));
                            emit connected(QString(name));
                            break;
                        }
                    }
                }
            }

            if (m_fd >= 0) {
                struct js_event e;
                while (::read(m_fd, &e, sizeof(e)) > 0) {
                    e.type &= ~JS_EVENT_INIT;
                    if (e.type == JS_EVENT_AXIS) {
                        float val = e.value / 32767.0f;
                        emit axisChanged(e.number, val);
                    } else if (e.type == JS_EVENT_BUTTON) {
                        emit buttonChanged(e.number, e.value != 0);
                    }
                }
                
                // Check if disconnected
                if (errno != EAGAIN) {
                    ::close(m_fd);
                    m_fd = -1;
                    emit disconnected();
                }
            }

            QThread::msleep(50); // Poll rate
        }
        
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
#else
        // Dummy implementation for Windows
        while (m_running) {
            QThread::msleep(500);
        }
#endif
    }

    void stop() {
        m_running = false;
    }

signals:
    void connected(const QString& name);
    void disconnected();
    void axisChanged(int axis, float value);
    void buttonChanged(int button, bool pressed);

private:
    bool m_running;
    int m_fd;
};

JoystickHandler::JoystickHandler(QObject *parent)
    : QObject(parent), m_connected(false), m_isLearning(false)
{
    loadMapping();

    m_thread = new QThread(this);
    m_worker = new JoystickWorker();
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &JoystickWorker::start);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(m_worker, &JoystickWorker::connected, this, &JoystickHandler::onWorkerConnected, Qt::QueuedConnection);
    connect(m_worker, &JoystickWorker::disconnected, this, &JoystickHandler::onWorkerDisconnected, Qt::QueuedConnection);
    connect(m_worker, &JoystickWorker::axisChanged, this, &JoystickHandler::onRawAxis, Qt::QueuedConnection);
    connect(m_worker, &JoystickWorker::buttonChanged, this, &JoystickHandler::onRawButton, Qt::QueuedConnection);

    m_thread->start();
}

JoystickHandler::~JoystickHandler() {
    if (m_thread->isRunning()) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::QueuedConnection);
        m_thread->quit();
        m_thread->wait();
    }
}

void JoystickHandler::setMapping(const QVariantMap& map) {
    m_mapping = map;
    emit mappingChanged();
}

void JoystickHandler::setIsLearning(bool learning) {
    if (m_isLearning != learning) {
        m_isLearning = learning;
        emit isLearningChanged();
    }
}

void JoystickHandler::refreshDevices() {
    // Handled by worker loop automatically reconnecting
}

void JoystickHandler::onWorkerConnected(const QString& name) {
    m_deviceName = name;
    m_connected = true;
    emit deviceNameChanged();
    emit connectedChanged();
}

void JoystickHandler::onWorkerDisconnected() {
    m_deviceName = "";
    m_connected = false;
    emit deviceNameChanged();
    emit connectedChanged();
}

void JoystickHandler::onRawAxis(int axis, float value) {
    emit rawAxisChanged(axis, value);
    
    QString inputName = QString("Axis_%1").arg(axis);
    
    if (m_isLearning) {
        if (qAbs(value) > 0.5f) { // Threshold for learning
            m_lastLearnedInput = inputName;
            emit lastLearnedInputChanged();
        }
        return;
    }

    // Apply mapping
    for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
        if (it.value().toString() == inputName) {
            emit mappedAxisChanged(it.key(), value);
        }
    }
}

void JoystickHandler::onRawButton(int button, bool pressed) {
    emit rawButtonChanged(button, pressed);
    
    QString inputName = QString("Button_%1").arg(button);
    
    if (m_isLearning) {
        if (pressed) {
            m_lastLearnedInput = inputName;
            emit lastLearnedInputChanged();
        }
        return;
    }

    // Apply mapping
    for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
        if (it.value().toString() == inputName) {
            emit mappedButtonChanged(it.key(), pressed);
        }
    }
}

void JoystickHandler::loadMapping() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreESP", "LibreESPBot");
    QVariant mapVar = settings.value("Joystick/Mapping");
    if (mapVar.isValid()) {
        m_mapping = mapVar.toMap();
    } else {
        // Default Xbox/PS4-ish mapping
        m_mapping["Throttle"] = "Axis_1"; // Left Y
        m_mapping["Steering"] = "Axis_3"; // Right X or Left X (usually 0 or 3)
        m_mapping["Brake"] = "Button_0";  // A / Cross
        m_mapping["Reverse"] = "Button_1"; // B / Circle
        m_mapping["Radar"] = "Button_2";  // X / Square
        m_mapping["SpeedMode"] = "Button_3"; // Y / Triangle
    }
}

void JoystickHandler::saveMapping() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreESP", "LibreESPBot");
    settings.setValue("Joystick/Mapping", m_mapping);
}

#include "JoystickHandler.moc"
