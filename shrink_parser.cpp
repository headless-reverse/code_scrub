#include "shrink_parser.h"
#include <QVarLengthArray>
#include <QSet>
#include <QMap>
#include <QRegularExpression>
#include <cstring>
#include <algorithm> 

static QString shrinkFallback(const QString& code, CodeLanguage lang, const ShrinkOptions& opts) {
    QString out;
    out.reserve(code.size());
    int i = 0, len = code.size();
    bool inString = false;
    QChar stringChar;
    bool inTripleString = false;
    QString tripleDelimiter;

    if (lang == CodeLanguage::Cpp || lang == CodeLanguage::C || lang == CodeLanguage::Java ||
        lang == CodeLanguage::JavaScript || lang == CodeLanguage::Css) {
        while (i < len) {
            int currentLine = code.left(i).count('\n') + 1;
            if (opts.excluded_lines.contains(currentLine)) {
                while (i < len) {
                    out.append(code[i]);
                    if (code[i] == '\n') { i++; break; }
                    i++;
                }
                continue;
            }
            if (inString && code[i] == '\\' && i + 1 < len) {
                out.append(code[i]); out.append(code[i+1]); i += 2; continue;
            }
            if (!inString && (code[i] == '"' || code[i] == '\'')) {
                inString = true; stringChar = code[i]; out.append(code[i]); i++; continue;
            }
            if (inString && code[i] == stringChar) { inString = false; out.append(code[i]); i++; continue; }
            if (!inString) {
                if (opts.remove_comments && code[i] == '/' && i + 1 < len && code[i+1] == '*') {
                    i += 2;
                    while (i < len) {
                        if (code[i] == '*' && i + 1 < len && code[i+1] == '/') { i += 2; break; }
                        i++;
                    }
                    continue;
                }
                if (opts.remove_comments && code[i] == '/' && i + 1 < len && code[i+1] == '/') {
                    i += 2;
                    while (i < len && code[i] != '\n' && code[i] != '\r') i++;
                    continue;
                }
            }
            out.append(code[i]); i++;
        }
    } else if (lang == CodeLanguage::Html) {
        while (i < len) {
            int currentLine = code.left(i).count('\n') + 1;
            if (opts.excluded_lines.contains(currentLine)) {
                while (i < len) {
                    out.append(code[i]);
                    if (code[i] == '\n') { i++; break; }
                    i++;
                }
                continue;
            }
            if (opts.remove_comments && i + 3 < len && code.mid(i, 4) == "<!--") {
                i += 4;
                while (i + 2 < len && code.mid(i, 3) != "-->") i++;
                i = qMin(i + 3, len);
                continue;
            }
            out.append(code[i]); i++;
        }
    } else if (lang == CodeLanguage::Python) {
        while (i < len) {
            int currentLine = code.left(i).count('\n') + 1;
            if (opts.excluded_lines.contains(currentLine)) {
                while (i < len) {
                    out.append(code[i]);
                    if (code[i] == '\n') { i++; break; }
                    i++;
                }
                continue;
            }
            if ((inString || inTripleString) && code[i] == '\\' && i + 1 < len) {
                out.append(code[i]); out.append(code[i+1]); i += 2; continue;
            }
            if (!inString && !inTripleString) {
                if (i + 2 < len && (code.mid(i, 3) == "\"\"\"" || code.mid(i, 3) == "'''")) {
                    tripleDelimiter = code.mid(i, 3);
                    if (opts.remove_docstrings) {
                        i += 3;
                        while (i < len) {
                            if (i + 2 < len && code.mid(i, 3) == tripleDelimiter) { i += 3; break; }
                            i++;
                        }
                        continue;
                    } else {
                        inTripleString = true; out.append(tripleDelimiter); i += 3; continue;
                    }
                }
            }
            if (inTripleString) {
                if (i + 2 < len && code.mid(i, 3) == tripleDelimiter) {
                    inTripleString = false; out.append(tripleDelimiter); i += 3; continue;
                }
                out.append(code[i]); i++; continue;
            }
            if (!inString && (code[i] == '"' || code[i] == '\'')) {
                inString = true; stringChar = code[i]; out.append(code[i]); i++; continue;
            }
            if (inString && code[i] == stringChar) { inString = false; out.append(code[i]); i++; continue; }
            if (!inString && !inTripleString && opts.remove_comments && code[i] == '#') {
                while (i < len && code[i] != '\n' && code[i] != '\r') i++;
                continue;
            }
            out.append(code[i]); i++;
        }
    }
    if (lang == CodeLanguage::Cpp || lang == CodeLanguage::C) {
        if (opts.remove_pragmas) {
            out.remove(QRegularExpression(R"((?m)^\s*#\s*pragma[^\n]*(\n|$))"));
        }
    } else if (lang == CodeLanguage::Java) {
        if (opts.remove_docstrings) {
            out.remove(QRegularExpression(R"(/\*\*[\s\S]*?\*/)"));
        }
        if (opts.remove_annotations) {
            out.remove(QRegularExpression(R"((?m)^\s*@[\w.]+(?:\([^)]*\))?\s*(\n|$))"));
        }
    }

    return out;
}

ShrinkParser::ShrinkParser(const QString& code, CodeLanguage lang) 
    : m_code(code), m_lang(lang) {
    m_parser = ts_parser_new();
    m_sourceBytes = code.toUtf8();

    const TSLanguage* tsLang = TreeSitterLoader::getLanguage(m_lang);

    if (tsLang && m_parser) {
        ts_parser_set_language(m_parser, tsLang);
        m_tree = ts_parser_parse_string(m_parser, nullptr, m_sourceBytes.constData(), m_sourceBytes.size());
    }
}

ShrinkParser::~ShrinkParser() {
    if (m_tree) ts_tree_delete(m_tree);
    if (m_parser) ts_parser_delete(m_parser);
}

int ShrinkParser::lineForByte(uint32_t byteOffset) const {
    if (byteOffset > static_cast<uint32_t>(m_sourceBytes.size())) byteOffset = m_sourceBytes.size();
    return m_sourceBytes.left(byteOffset).count('\n') + 1;
}

QString ShrinkParser::process(const ShrinkOptions& opts) {
    if (!m_tree) { return shrinkFallback(m_code, m_lang, opts); }

    int len = m_sourceBytes.size();
    QVarLengthArray<bool> keepMask(len);
    for (int i = 0; i < len; ++i) keepMask[i] = true;

    TSNode root = ts_tree_root_node(m_tree);
    executeASTFiltering(root, opts, keepMask);

    QByteArray intermediate;
    intermediate.reserve(len);
    for (int i = 0; i < len; ++i) { if (keepMask[i]) intermediate.append(m_sourceBytes[i]); }

    QString textResult = QString::fromUtf8(intermediate);

    if (opts.obfuscate_locals) {
        ShrinkParser subParser(textResult, m_lang);
        if (subParser.m_tree) {
            TSNode subRoot = ts_tree_root_node(subParser.m_tree);
            subParser.performObfuscation(subRoot, textResult);
        }
    }

    return textResult;
}

void ShrinkParser::executeASTFiltering(TSNode node, const ShrinkOptions& opts, QVarLengthArray<bool>& mask) {
    const char* type = ts_node_type(node);
    uint32_t sb = ts_node_start_byte(node);
    uint32_t eb = ts_node_end_byte(node);

    bool eraseNode = false;
    const LanguageDefinition& def = LanguageRegistry::getDefinition(m_lang);
    const int startLine = lineForByte(sb);

    if (opts.excluded_lines.contains(startLine)) {
        return;
    }

    if (opts.remove_unused_functions &&
        opts.unused_function_lines.contains(startLine) &&
        !def.functionNode.isEmpty() &&
        strcmp(type, def.functionNode.toUtf8().constData()) == 0) {
        eraseNode = true;
    }

    if (strcmp(type, def.commentNode.toUtf8().constData()) == 0 && opts.remove_comments) { eraseNode = true; }

    if (m_lang == CodeLanguage::Python) {
        if (strcmp(type, "expression_statement") == 0 && opts.remove_docstrings) {
            TSNode child = ts_node_child(node, 0);
            if (!ts_node_is_null(child) && strcmp(ts_node_type(child), "string") == 0) { eraseNode = true; }
        }
        if (strcmp(type, "type") == 0 && opts.remove_type_hints) { eraseNode = true; }
    } else if (m_lang == CodeLanguage::Java) {
        if (strcmp(type, "comment") == 0 && opts.remove_docstrings) {
            const QByteArray text = m_sourceBytes.mid(sb, eb - sb);
            if (text.startsWith("/**")) { eraseNode = true; }
        }
        if (strcmp(type, "marker_annotation") == 0 && opts.remove_annotations) { eraseNode = true; }
        if (strcmp(type, "annotation") == 0 && opts.remove_annotations) { eraseNode = true; }
    }

    if (eraseNode) {
        for (uint32_t i = sb; i < eb; ++i) {
            if (i < static_cast<uint32_t>(mask.size())) mask[i] = false;
        }
        return;
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) { executeASTFiltering(ts_node_child(node, i), opts, mask); }
}

void ShrinkParser::performObfuscation(TSNode node, QString& codeText) {
    const char* type = ts_node_type(node);
    const LanguageDefinition& def = LanguageRegistry::getDefinition(m_lang);

    if (strcmp(type, def.functionNode.toUtf8().constData()) == 0) {
        QMap<QString, QString> renameMap;
        int variableCounter = 0;

        std::function<void(TSNode)> findDeclarations = [&](TSNode n) {
            const char* nType = ts_node_type(n);
            if (strcmp(nType, def.variableNode.toUtf8().constData()) == 0 ||
                strcmp(nType, def.parameterNode.toUtf8().constData()) == 0) {
                
                TSNode nameNode = ts_node_child_by_field_name(n, "name", 4);
                if (ts_node_is_null(nameNode) && ts_node_child_count(n) > 0) { nameNode = ts_node_child(n, 0); }

                if (!ts_node_is_null(nameNode) && strcmp(ts_node_type(nameNode), "identifier") == 0) {
                    uint32_t sb = ts_node_start_byte(nameNode);
                    uint32_t eb = ts_node_end_byte(nameNode);
                    QString varName = QString::fromUtf8(m_sourceBytes.mid(sb, eb - sb));
                    if (!renameMap.contains(varName) && !def.keywords.contains(varName)) {
                        renameMap[varName] = QString("v_%1").arg(variableCounter++);
                    }
                }
            }
            uint32_t cc = ts_node_child_count(n);
            for (uint32_t i = 0; i < cc; ++i) findDeclarations(ts_node_child(n, i));
        };

        findDeclarations(node);

        if (!renameMap.isEmpty()) {
            struct Replacement {
                int start;
                int len;
                QString newName;
            };
            QVector<Replacement> replacements;

            std::function<void(TSNode)> applyRename = [&](TSNode n) {
                if (strcmp(ts_node_type(n), "identifier") == 0) {
                    uint32_t sb = ts_node_start_byte(n);
                    uint32_t eb = ts_node_end_byte(n);
                    QString text = QString::fromUtf8(m_sourceBytes.mid(sb, eb - sb));
                    if (renameMap.contains(text)) {
                        replacements.append({static_cast<int>(sb), static_cast<int>(eb - sb), renameMap[text]});
                    }
                }
                uint32_t cc = ts_node_child_count(n);
                for (uint32_t i = 0; i < cc; ++i) applyRename(ts_node_child(n, i));
            };

            applyRename(node);

            std::sort(replacements.begin(), replacements.end(), [](const Replacement& a, const Replacement& b) {
                return a.start > b.start;
            });

            QByteArray workingBuffer = codeText.toUtf8();
            for (const auto& rep : replacements) {
                workingBuffer.replace(rep.start, rep.len, rep.newName.toUtf8());
            }
            codeText = QString::fromUtf8(workingBuffer);
        }
        return;
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) { performObfuscation(ts_node_child(node, i), codeText); }
}

QStringList ShrinkParser::detectUnusedVariables() {
    QStringList unusedList;
    if (!m_tree) return unusedList;

    TSNode root = ts_tree_root_node(m_tree);
    const LanguageDefinition& def = LanguageRegistry::getDefinition(m_lang);

    QMap<QString, int> references;
    QVector<QString> declared;

    std::function<void(TSNode)> collectStats = [&](TSNode n) {
        const char* type = ts_node_type(n);
        if (strcmp(type, "identifier") == 0) {
            uint32_t sb = ts_node_start_byte(n);
            uint32_t eb = ts_node_end_byte(n);
            QString text = QString::fromUtf8(m_sourceBytes.mid(sb, eb - sb));
            references[text]++;
        }
        if (strcmp(type, def.variableNode.toUtf8().constData()) == 0) {
            TSNode nameNode = ts_node_child_by_field_name(n, "name", 4);
            if (!ts_node_is_null(nameNode)) {
                uint32_t sb = ts_node_start_byte(nameNode);
                uint32_t eb = ts_node_end_byte(nameNode);
                declared.append(QString::fromUtf8(m_sourceBytes.mid(sb, eb - sb)));
            }
        }
        uint32_t cc = ts_node_child_count(n);
        for (uint32_t i = 0; i < cc; ++i) collectStats(ts_node_child(n, i));
    };

    collectStats(root);

    for (const auto& var : declared) {
        if (references[var] <= 1) { unusedList.append(var); }
    }

    return unusedList;
}
