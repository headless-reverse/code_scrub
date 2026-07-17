#include "ast_highlighter.h"

AstHighlighter::AstHighlighter(QTextDocument *parent, CodeLanguage lang)
    : QSyntaxHighlighter(parent), m_lang(lang) {
    
    m_keywordFormat.setForeground(QColor("#0000FF"));
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_commentFormat.setForeground(QColor("#008000"));
    m_commentFormat.setFontItalic(true);

    m_stringFormat.setForeground(QColor("#A31515"));

    m_functionFormat.setForeground(QColor("#745310"));
    m_functionFormat.setFontWeight(QFont::Bold);

    m_typeFormat.setForeground(QColor("#2B91AF"));
}

void AstHighlighter::updateTree(TSTree* tree, const QByteArray& sourceBytes) {
    m_tree = tree;
    m_source = sourceBytes;
    rehighlight();
}

void AstHighlighter::highlightBlock(const QString &text) {
    if (m_tree) {
        highlightUsingAST(text, currentBlock().position());
    } else { highlightFallback(text); }
}

void AstHighlighter::highlightUsingAST(const QString &text, int blockStart) {
    int blockLen = text.length();
    int blockEnd = blockStart + blockLen;

    TSNode root = ts_tree_root_node(m_tree);
    
    std::function<void(TSNode)> traverse = [&](TSNode node) {
        uint32_t sb = ts_node_start_byte(node);
        uint32_t eb = ts_node_end_byte(node);

        if (sb >= static_cast<uint32_t>(blockEnd) || eb <= static_cast<uint32_t>(blockStart)) { return; }

        const char* type = ts_node_type(node);
        const LanguageDefinition& def = LanguageRegistry::getDefinition(m_lang);

        if (strcmp(type, def.commentNode.toUtf8().constData()) == 0) {
            setFormat(sb - blockStart, eb - sb, m_commentFormat);
        } else if (strcmp(type, "string") == 0 || strcmp(type, "string_literal") == 0) {
            setFormat(sb - blockStart, eb - sb, m_stringFormat);
        } else if (strcmp(type, "identifier") == 0) {
            QString token = QString::fromUtf8(m_source.mid(sb, eb - sb));
            if (def.keywords.contains(token)) { setFormat(sb - blockStart, eb - sb, m_keywordFormat); }
        } else if (strcmp(type, "type_identifier") == 0 || strcmp(type, "primitive_type") == 0) { setFormat(sb - blockStart, eb - sb, m_typeFormat); }

        uint32_t childCount = ts_node_child_count(node);
        for (uint32_t i = 0; i < childCount; ++i) { traverse(ts_node_child(node, i)); }
    };

    traverse(root);
}

void AstHighlighter::highlightFallback(const QString &text) {
    const LanguageDefinition& def = LanguageRegistry::getDefinition(m_lang);
    for (const QString& kw : def.keywords) {
        QRegularExpression regex(R"(\b)" + QRegularExpression::escape(kw) + R"(\b)");
        QRegularExpressionMatchIterator i = regex.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), m_keywordFormat);
        }
    }
}
