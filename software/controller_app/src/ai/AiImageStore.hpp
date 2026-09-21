#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QMap>
#include <QStringList>
#include <QAbstractListModel>
#include <QDateTime>

class AiMemoryModel;

class AiImageStore : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QObject* memoryModel READ memoryModel CONSTANT)
public:
    enum Roles {
        FilenameRole = Qt::UserRole + 1,
        FilepathRole,
        TimestampRole
    };

    explicit AiImageStore(QObject* parent = nullptr);

    // QAbstractListModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // In-memory "memory" items model (key, description, thumbnail filepath),
    // for things stored with save_image_to_memory.
    QObject* memoryModel() const { return m_memoryModel; }

    // C++ API for the Dispatcher
    void saveImage(const QImage& img, const QString& label);
    void saveImageToMemory(const QImage& img, const QString& key);
    QString recallImageAsBase64(const QString& key);
    QImage getImageFromMemory(const QString& key) const;

    // Vision descriptions: each stored memory image gets a text description so
    // text-only models (e.g. DeepSeek) can still "see" and "recall" it.
    QString recallDescription(const QString& key) const;
    void setMemoryDescription(const QString& key, const QString& description);

    // QML API
    Q_INVOKABLE void refreshGallery();
    Q_INVOKABLE void setSaveDirectory(const QString& path);

private:
    QString m_saveDir;
    QMap<QString, QImage> m_memoryImages;
    QMap<QString, QString> m_memoryDescriptions;
    QObject* m_memoryModel = nullptr;
    
    struct GalleryItem {
        QString filename;
        QString filepath;
        QDateTime timestamp;
    };
    QList<GalleryItem> m_gallery;
};
