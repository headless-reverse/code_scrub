#pragma once

#include <QThread>
#include <QStringList>

class ArchiveWorker : public QThread {
    Q_OBJECT
public:
    ArchiveWorker(const QStringList& files, const QString& archivePath, QObject* parent = nullptr);

signals:
    void progressUpdated(int percent);
    void finished(bool success, const QString& message);

protected:
    void run() override;

private:
    QStringList m_files;
    QString m_archivePath;
};
