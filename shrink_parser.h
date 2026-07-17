#pragma once

#include <QString>
#include <QByteArray>
#include <QVarLengthArray>
#include <tree_sitter/api.h>
#include "shrink.h"
#include "language.h"
#include "analysis_engine.h"

class ShrinkParser {
public:
    ShrinkParser(const QString& code, CodeLanguage lang);
    ~ShrinkParser();

    QString process(const ShrinkOptions& opts);
    QStringList detectUnusedVariables();

private:
    QString m_code;
    QByteArray m_sourceBytes;
    CodeLanguage m_lang;
    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;

    void executeASTFiltering(TSNode node, const ShrinkOptions& opts, QVarLengthArray<bool>& mask);
    void performObfuscation(TSNode node, QString& codeText);
};
