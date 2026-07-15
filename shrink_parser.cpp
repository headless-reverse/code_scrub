#include "shrink_parser.h"
#include <QVarLengthArray>
#include <cstring>

static QString shrinkFallback(const QString& code, CodeLanguage lang, const ShrinkOptions& opts) {
    QString out;
    out.reserve(code.size());
    int i = 0;
    int len = code.size();

    bool inString = false;
    QChar stringChar;
    bool inTripleString = false;
    QString tripleDelimiter;

    if (lang == CodeLanguage::Cpp) {
        while (i < len) {
            if (inString && code[i] == '\\' && i + 1 < len) {
                out.append(code[i]);
                out.append(code[i+1]);
                i += 2;
                continue;
            }
            if (!inString && (code[i] == '"' || code[i] == '\'')) {
                inString = true;
                stringChar = code[i];
                out.append(code[i]);
                i++;
                continue;
            }
            if (inString && code[i] == stringChar) {
                inString = false;
                out.append(code[i]);
                i++;
                continue;
            }

            if (!inString) {
                if (opts.remove_comments && code[i] == '/' && i + 1 < len && code[i+1] == '*') {
                    i += 2;
                    while (i < len) {
                        if (code[i] == '*' && i + 1 < len && code[i+1] == '/') {
                            i += 2;
                            break;
                        }
                        i++;
                    }
                    continue;
                }
                if (opts.remove_comments && code[i] == '/' && i + 1 < len && code[i+1] == '/') {
                    i += 2;
                    while (i < len && code[i] != '\n' && code[i] != '\r') {
                        i++;
                    }
                    continue;
                }
            }

            out.append(code[i]);
            i++;
        }
    } else if (lang == CodeLanguage::Python) {
        while (i < len) {
            if ((inString || inTripleString) && code[i] == '\\' && i + 1 < len) {
                out.append(code[i]);
                out.append(code[i+1]);
                i += 2;
                continue;
            }

            if (!inString && !inTripleString) {
                if (i + 2 < len && (code.mid(i, 3) == "\"\"\"" || code.mid(i, 3) == "'''")) {
                    tripleDelimiter = code.mid(i, 3);
                    if (opts.remove_docstrings) {
                        i += 3;
                        while (i < len) {
                            if (i + 2 < len && code.mid(i, 3) == tripleDelimiter) {
                                i += 3;
                                break;
                            }
                            i++;
                        }
                        continue;
                    } else {
                        inTripleString = true;
                        out.append(tripleDelimiter);
                        i += 3;
                        continue;
                    }
                }
            }

            if (inTripleString) {
                if (i + 2 < len && code.mid(i, 3) == tripleDelimiter) {
                    inTripleString = false;
                    out.append(tripleDelimiter);
                    i += 3;
                    continue;
                }
                out.append(code[i]);
                i++;
                continue;
            }

            if (!inString && (code[i] == '"' || code[i] == '\'')) {
                inString = true;
                stringChar = code[i];
                out.append(code[i]);
                i++;
                continue;
            }
            if (inString && code[i] == stringChar) {
                inString = false;
                out.append(code[i]);
                i++;
                continue;
            }

            if (!inString && !inTripleString && opts.remove_comments && code[i] == '#') {
                while (i < len && code[i] != '\n' && code[i] != '\r') {
                    i++;
                }
                continue;
            }

            out.append(code[i]);
            i++;
        }
    } else {
        return code;
    }
    return out;
}

ShrinkParser::ShrinkParser(const QString& code, CodeLanguage lang) 
    : m_code(code), m_lang(lang) {
    m_parser = ts_parser_new();
    m_sourceBytes = code.toUtf8();

    const TSLanguage* tsLang = nullptr;
    if (m_lang == CodeLanguage::Cpp) {
        tsLang = TreeSitterLoader::getCppLanguage();
    } else if (m_lang == CodeLanguage::Python) {
        tsLang = TreeSitterLoader::getPythonLanguage();
    }

    if (tsLang && m_parser) {
        ts_parser_set_language(m_parser, tsLang);
        m_tree = ts_parser_parse_string(m_parser, nullptr, m_sourceBytes.constData(), m_sourceBytes.size());
    }
}

ShrinkParser::~ShrinkParser() {
    if (m_tree) {
        ts_tree_delete(m_tree);
    }
    if (m_parser) {
        ts_parser_delete(m_parser);
    }
}

QString ShrinkParser::process(const ShrinkOptions& opts) {
    if (!m_tree) {
        return shrinkFallback(m_code, m_lang, opts);
    }

    int len = m_sourceBytes.size();
    QVarLengthArray<bool> keepMask(len);
    for (int i = 0; i < len; ++i) {
        keepMask[i] = true;
    }

    TSNode root = ts_tree_root_node(m_tree);
    executeASTFiltering(root, opts, keepMask);

    QByteArray output;
    output.reserve(len);
    for (int i = 0; i < len; ++i) {
        if (keepMask[i]) {
            output.append(m_sourceBytes[i]);
        }
    }

    return QString::fromUtf8(output);
}

void ShrinkParser::executeASTFiltering(TSNode node, const ShrinkOptions& opts, QVarLengthArray<bool>& mask) {
    const char* type = ts_node_type(node);
    uint32_t sb = ts_node_start_byte(node);
    uint32_t eb = ts_node_end_byte(node);

    bool eraseNode = false;

    if (strcmp(type, "comment") == 0 && opts.remove_comments) {
        eraseNode = true;
    }

    if (m_lang == CodeLanguage::Python) {
        if (strcmp(type, "expression_statement") == 0 && opts.remove_docstrings) {
            TSNode child = ts_node_child(node, 0);
            if (!ts_node_is_null(child) && strcmp(ts_node_type(child), "string") == 0) {
                eraseNode = true;
            }
        }
        if (strcmp(type, "type") == 0 && opts.remove_type_hints) {
            eraseNode = true;
        }
    }

    if (eraseNode) {
        for (uint32_t i = sb; i < eb; ++i) {
            if (i < static_cast<uint32_t>(mask.size())) {
                mask[i] = false;
            }
        }
        return;
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        executeASTFiltering(ts_node_child(node, i), opts, mask);
    }
}
