#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include "language.h"
#include <tree_sitter/api.h>

class AstHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit AstHighlighter(QTextDocument *parent, CodeLanguage lang);
    void updateTree(TSTree* tree, const QByteArray& sourceBytes);

protected:
    void highlightBlock(const QString &text) override;

private:
    CodeLanguage m_lang;
    TSTree* m_tree = nullptr;
    QByteArray m_source;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_typeFormat;

    void highlightUsingAST(const QString &text, int blockStart);
    void highlightFallback(const QString &text);
};
