#include "AiImageStore.hpp"
#include "AiVision.hpp"
#include <QDir>
#include <QStandardPaths>
#include <QBuffer>
#include <QDebug>
#include <QFileInfoList>
#include <QFileInfo>
#include <QDateTime>

// Lightweight in-memory list model for images stored via save_image_to_memory.
class AiMemoryModel : public QAbstractListModel {
public:
    enum Roles { KeyRole = Qt::UserRole + 1, DescRole, FilepathRole, TimeRole };

    AiMemoryModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_keys.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= m_keys.size()) return QVariant();
        int r = index.row();
        switch (role) {
            case KeyRole:      return m_keys[r];
            case DescRole:     return m_descs[r];
            case FilepathRole: return "file:///" + m_files[r];
            case TimeRole:     return m_times[r];
        }
        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{KeyRole, "key"}, {DescRole, "description"},
                {FilepathRole, "filepath"}, {TimeRole, "time"}};
    }

    void addEntry(const QString& key, const QString& desc, const QString& file, const QString& time) {
        beginInsertRows(QModelIndex(), m_keys.size(), m_keys.size());
        m_keys.prepend(key);
        m_descs.prepend(desc);
        m_files.prepend(file);
        m_times.prepend(time);
        endInsertRows();
    }

private:
    QStringList m_keys;
    QStringList m_descs;
    QStringList m_files;
    QStringList m_times;
};

AiImageStore::AiImageStore(QObject* parent) : QAbstractListModel(parent) {
    m_memoryModel = new AiMemoryModel(this);
    m_saveDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/LibreESPBot";
    QDir().mkpath(m_saveDir);
    QDir().mkpath(m_saveDir + "/memory");
    refreshGallery();
}

int AiImageStore::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_gallery.size();
}

QVariant AiImageStore::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_gallery.size()) return QVariant();

    const GalleryItem& item = m_gallery[index.row()];
    switch (role) {
        case FilenameRole: return item.filename;
        case FilepathRole: return "file:///" + item.filepath; // for QML Image source
        case TimestampRole: return item.timestamp;
    }
    return QVariant();
}

QHash<int, QByteArray> AiImageStore::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[FilenameRole] = "filename";
    roles[FilepathRole] = "filepath";
    roles[TimestampRole] = "timestamp";
    return roles;
}

void AiImageStore::setSaveDirectory(const QString& path) {
    m_saveDir = path;
    QDir().mkpath(m_saveDir);
    refreshGallery();
}

void AiImageStore::refreshGallery() {
    beginResetModel();
    m_gallery.clear();

    QDir dir(m_saveDir);
    dir.setNameFilters(QStringList() << "*.jpg" << "*.png");
    dir.setSorting(QDir::Time | QDir::Reversed); // Newest first

    QFileInfoList list = dir.entryInfoList();
    for (const QFileInfo& fileInfo : list) {
        GalleryItem item;
        item.filename = fileInfo.fileName();
        item.filepath = fileInfo.absoluteFilePath();
        item.timestamp = fileInfo.lastModified();
        m_gallery.append(item);
    }
    endResetModel();
}

void AiImageStore::saveImage(const QImage& img, const QString& label) {
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString filename = label + "_" + ts + ".jpg";
    QString filepath = m_saveDir + "/" + filename;

    if (img.save(filepath, "JPG", 90)) {
        qDebug() << "[AiImageStore] Saved image to:" << filepath;
        refreshGallery();
    } else {
        qWarning() << "[AiImageStore] Failed to save image:" << filepath;
    }
}

void AiImageStore::saveImageToMemory(const QImage& img, const QString& key) {
    m_memoryImages[key] = img;
    QString description = AiVision::describe(img); // "eyes" for text-only models
    m_memoryDescriptions[key] = description;

    // Write a small thumbnail so the UI can display memory items.
    QImage thumb = img.scaled(QSize(480, 480), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QString file = m_saveDir + "/memory/" + key + ".jpg";
    if (!thumb.save(file, "JPG", 80)) {
        qWarning() << "[AiImageStore] Failed to write memory thumbnail:" << file;
        file = QString();
    }

    if (auto* mm = static_cast<AiMemoryModel*>(m_memoryModel)) {
        mm->addEntry(key, description, file,
                     QDateTime::currentDateTime().toString("hh:mm:ss"));
    }

    qDebug() << "[AiImageStore] Saved memory image under key:" << key;
}

void AiImageStore::setMemoryDescription(const QString& key, const QString& description) {
    m_memoryDescriptions[key] = description;
}

QString AiImageStore::recallDescription(const QString& key) const {
    if (m_memoryDescriptions.contains(key)) {
        return m_memoryDescriptions[key];
    }
    if (m_memoryImages.contains(key)) {
        return AiVision::describe(m_memoryImages[key]);
    }
    return QStringLiteral("<no stored image or description for key '%1'>").arg(key);
}

QString AiImageStore::recallImageAsBase64(const QString& key) {
    if (!m_memoryImages.contains(key)) {
        qWarning() << "[AiImageStore] Recall failed: no memory image for key:" << key;
        return QString();
    }

    QImage img = m_memoryImages[key];
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPG", 80);
    return QString(ba.toBase64());
}

QImage AiImageStore::getImageFromMemory(const QString& key) const {
    return m_memoryImages.value(key, QImage());
}
