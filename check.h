#pragma once

#include <QThread>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QMetaType>

struct FunctionResult {
    QString signature;
    QString file;
    int line;
    QString status;
    int count;
    QString context;
    QString locations;
};

struct FileResult {
    QString file;
    QString fullPath;
    QString type;
    QString status;
    int count;
    QString locations;
};

Q_DECLARE_METATYPE(FunctionResult)
Q_DECLARE_METATYPE(FileResult)

class FunctionAnalysisWorker : public QThread {
    Q_OBJECT
public:
    explicit FunctionAnalysisWorker(const QStringList& files, QObject* parent = nullptr);
    void cancel();

signals:
    void progressUpdated(int percentage);
    void resultsReady(const QVector<FunctionResult>& results, int total, int unused, int duplicates);
    void cancelled();

protected:
    void run() override;

private:
    QStringList m_files;
    bool m_cancelRequested;

    QString stripCommentsAndStrings(const QString& text);
    QString extractFullBody(const QString& rawText, int startPos);
};

class FileAnalysisWorker : public QThread {
    Q_OBJECT
public:
    explicit FileAnalysisWorker(const QStringList& files, QObject* parent = nullptr);
    void cancel();

signals:
    void progressUpdated(int percentage);
    void resultsReady(const QVector<FileResult>& results);
    void cancelled();

protected:
    void run() override;

private:
    QStringList m_files;
    bool m_cancelRequested;
};
