#include "check.h"
#include "analysis_engine.h"

FunctionAnalysisWorker::FunctionAnalysisWorker(const QStringList& files, QObject* parent)
    : QThread(parent), m_files(files), m_cancelRequested(false) {
    qRegisterMetaType<QVector<FunctionResult>>("QVector<FunctionResult>");
}

void FunctionAnalysisWorker::cancel() {
    m_cancelRequested = true;
}

void FunctionAnalysisWorker::run() {
    AnalysisEngine engine;
    engine.setFiles(m_files);

    auto progressRelay = [this](int value) {
        emit progressUpdated(value);
    };

    auto cancelCheck = [this]() -> bool {
        return m_cancelRequested;
    };

    if (engine.runAnalysis(progressRelay, cancelCheck)) {
        emit resultsReady(engine.getFunctionResults(), 
                           engine.getTotalFunctions(), 
                           engine.getUnusedFunctions(), 
                           engine.getDuplicateFunctions());
    } else {
        emit cancelled();
    }
}

FileAnalysisWorker::FileAnalysisWorker(const QStringList& files, QObject* parent)
    : QThread(parent), m_files(files), m_cancelRequested(false) {
    qRegisterMetaType<QVector<FileResult>>("QVector<FileResult>");
}

void FileAnalysisWorker::cancel() {
    m_cancelRequested = true;
}

void FileAnalysisWorker::run() {
    AnalysisEngine engine;
    engine.setFiles(m_files);

    auto progressRelay = [this](int value) {
        emit progressUpdated(value);
    };

    auto cancelCheck = [this]() -> bool {
        return m_cancelRequested;
    };

    if (engine.runAnalysis(progressRelay, cancelCheck)) {
        emit resultsReady(engine.getFileResults());
    } else {
        emit cancelled();
    }
}
