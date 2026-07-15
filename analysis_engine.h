#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <tree_sitter/api.h>
#include "check.h"

enum class CodeLanguage {
    Cpp,
    Python,
    Unknown
};

class TreeSitterLoader {
public:
    static const TSLanguage* getCppLanguage();
    static const TSLanguage* getPythonLanguage();
};

class AnalysisEngine {
public:
    AnalysisEngine();
    ~AnalysisEngine();

    void setFiles(const QStringList& files);
    bool runAnalysis(std::function<void(int)> progressCallback, std::function<bool()> cancelCheck);

    QVector<FunctionResult> getFunctionResults() const { return m_functionResults; }
    QVector<FileResult> getFileResults() const { return m_fileResults; }
    int getTotalFunctions() const { return m_totalFunctions; }
    int getUnusedFunctions() const { return m_unusedFunctions; }
    int getDuplicateFunctions() const { return m_duplicateFunctions; }

    static CodeLanguage detectLanguage(const QString& filePath);

private:
    QStringList m_files;
    QVector<FunctionResult> m_functionResults;
    QVector<FileResult> m_fileResults;
    int m_totalFunctions = 0;
    int m_unusedFunctions = 0;
    int m_duplicateFunctions = 0;

    struct ParsedFile {
        QString path;
        QByteArray content;
        TSTree* tree = nullptr;
        CodeLanguage lang = CodeLanguage::Unknown;
    };

    QMap<QString, ParsedFile> m_parsedProject;
    TSParser* m_parser = nullptr;

    void clearParsedProject();
    void parseFile(const QString& path);
    void extractFunctionsFromAST(const ParsedFile& pf, QVector<FunctionResult>& definitions);
    void traverseAndFindFunctions(TSNode node, const ParsedFile& pf, QVector<FunctionResult>& definitions);
    
    void countOccurrences(FunctionResult& def, const QVector<ParsedFile>& allFiles);
    void traverseAndCountCalls(TSNode node, const QByteArray& source, const QString& shortName, const QString& defFile, int defLine, int& count, QStringList& locations, const QString& currentFileName);

    void runFallbackAnalysis(QVector<FunctionResult>& definitions);
};
