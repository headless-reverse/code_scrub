#include "archive_worker.h"
#include <QFile>
#include <QFileInfo>
#include <archive.h>
#include <archive_entry.h>

ArchiveWorker::ArchiveWorker(const QStringList& files, const QString& archivePath, QObject* parent)
    : QThread(parent), m_files(files), m_archivePath(archivePath) {}

void ArchiveWorker::run() {
    struct archive* a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);

    if (archive_write_open_filename(a, m_archivePath.toLocal8Bit().constData()) != ARCHIVE_OK) {
        emit finished(false, "Nie można otworzyć archiwum do zapisu.");
        archive_write_free(a);
        return;
    }

    int count = m_files.size();
    bool success = true;

    for (int i = 0; i < count; ++i) {
        QString filePath = m_files[i];
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) continue;
        QByteArray fileData = file.readAll();

        struct archive_entry* entry = archive_entry_new();
        QString relativeName = QFileInfo(filePath).fileName();
        archive_entry_set_pathname(entry, relativeName.toLocal8Bit().constData());
        archive_entry_set_size(entry, fileData.size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);

        if (archive_write_header(a, entry) != ARCHIVE_OK) {
            success = false;
            archive_entry_free(entry);
            break;
        }

        archive_write_data(a, fileData.constData(), fileData.size());
        archive_entry_free(entry);

        emit progressUpdated(static_cast<int>((static_cast<double>(i) / count) * 100));
    }

    archive_write_close(a);
    archive_write_free(a);

    if (success) {
        emit finished(true, "Projekt skompresowany pomyślnie.");
    } else {
        emit finished(false, "Wystąpiły błędy podczas zapisu plików.");
    }
}
