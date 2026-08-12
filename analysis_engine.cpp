#include "analysis_engine.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QLibrary>
#include <QRegularExpression>
#include <QDebug>
#include <map>

typedef const TSLanguage *(*TSLanguageProvider)(void);

struct TreeSitterLanguageSpec {
    QString libraryName;
    QString symbolName;
};

static TreeSitterLanguageSpec treeSitterSpec(CodeLanguage lang) {
    switch (lang) {
    case CodeLanguage::C:
        return {"tree-sitter-c", "tree_sitter_c"};
    case CodeLanguage::Cpp:
        return {"tree-sitter-cpp", "tree_sitter_cpp"};
    case CodeLanguage::Python:
        return {"tree-sitter-python", "tree_sitter_python"};
    case CodeLanguage::Java:
        return {"tree-sitter-java", "tree_sitter_java"};
    case CodeLanguage::JavaScript:
        return {"tree-sitter-javascript", "tree_sitter_javascript"};
    case CodeLanguage::Html:
        return {"tree-sitter-html", "tree_sitter_html"};
    case CodeLanguage::Css:
        return {"tree-sitter-css", "tree_sitter_css"};
    case CodeLanguage::Unknown:
        return {};
    }
    return {};
}

struct TreeSitterLanguageHandle {
    QLibrary library;
    TSLanguageProvider provider = nullptr;
    bool attempted = false;
};

const TSLanguage* TreeSitterLoader::getLanguage(CodeLanguage lang) {
    static std::map<CodeLanguage, TreeSitterLanguageHandle> handles;
    TreeSitterLanguageHandle& handle = handles[lang];

    if (!handle.attempted) {
        handle.attempted = true;
        const TreeSitterLanguageSpec spec = treeSitterSpec(lang);
        if (!spec.libraryName.isEmpty()) {
            handle.library.setFileName(spec.libraryName);
            if (handle.library.load()) {
                handle.provider = reinterpret_cast<TSLanguageProvider>(handle.library.resolve(spec.symbolName.toUtf8().constData()));
            }
        }
    }

    return handle.provider ? handle.provider() : nullptr;
}

const TSLanguage* TreeSitterLoader::getCppLanguage() {
    return getLanguage(CodeLanguage::Cpp);
}

const TSLanguage* TreeSitterLoader::getPythonLanguage() {
    return getLanguage(CodeLanguage::Python);
}

AnalysisEngine::AnalysisEngine() { m_parser = ts_parser_new(); }

AnalysisEngine::~AnalysisEngine() {
    clearParsedProject();
    if (m_parser) { ts_parser_delete(m_parser); }
}

void AnalysisEngine::clearParsedProject() {
    for (auto& item : m_parsedProject) { if (item.tree) { ts_tree_delete(item.tree); } }
    m_parsedProject.clear();
}

CodeLanguage AnalysisEngine::detectLanguage(const QString& filePath) {
    return LanguageRegistry::detectFromPath(filePath);
}

void AnalysisEngine::setFiles(const QStringList& files) { m_files = files; }

void AnalysisEngine::parseFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { return; }

    QByteArray content = file.readAll();
    CodeLanguage lang = detectLanguage(path);

    const TSLanguage* tsLang = TreeSitterLoader::getLanguage(lang);

    TSTree* tree = nullptr;
    if (tsLang && m_parser) {
        ts_parser_set_language(m_parser, tsLang);
        tree = ts_parser_parse_string(m_parser, nullptr, content.constData(), content.size());
    }

    m_parsedProject[path] = { path, content, tree, lang };
}

bool AnalysisEngine::runAnalysis(std::function<void(int)> progressCallback, std::function<bool()> cancelCheck) {
    clearParsedProject();
    m_functionResults.clear();
    m_fileResults.clear();
    
    int totalFiles = m_files.size();
    if (totalFiles == 0) return true;

    for (int i = 0; i < totalFiles; ++i) {
        if (cancelCheck()) return false;
        parseFile(m_files[i]);
        if (progressCallback) {
            progressCallback(static_cast<int>((static_cast<double>(i) / totalFiles) * 20));
        }
    }

    QVector<FunctionResult> rawDefinitions;
    bool anyASTLoaded = false;

    for (auto it = m_parsedProject.begin(); it != m_parsedProject.end(); ++it) {
        if (cancelCheck()) return false;
        if (it.value().tree) {
            anyASTLoaded = true;
            extractFunctionsFromAST(it.value(), rawDefinitions);
        }
    }

    if (!anyASTLoaded) { runFallbackAnalysis(rawDefinitions); }

    QVector<ParsedFile> parsedList;
    for (const auto& pf : m_parsedProject) { parsedList.append(pf); }

    int totalDefs = rawDefinitions.size();
    m_totalFunctions = totalDefs;
    m_unusedFunctions = 0;
    m_duplicateFunctions = 0;

    for (int i = 0; i < totalDefs; ++i) {
        if (cancelCheck()) return false;
        FunctionResult def = rawDefinitions[i];

        countOccurrences(def, parsedList);

        if (def.count == 0 && def.signature.section(' ', 1).section('(', 0, 0).trimmed() != "main") {
            def.status = "UNUSED";
            m_unusedFunctions++;
        } else { def.status = "OK"; }

        bool isDuplicate = false;
        for (const auto& other : rawDefinitions) {
            if (other.signature == def.signature && (other.file != def.file || other.line != def.line)) {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate) {
            def.status = "DUPLICATE";
            m_duplicateFunctions++;
        }

        m_functionResults.append(def);

        if (progressCallback) {
            progressCallback(20 + static_cast<int>((static_cast<double>(i) / totalDefs) * 60));
        }
    }

    for (int i = 0; i < totalFiles; ++i) {
        if (cancelCheck()) return false;
        QString fpath = m_files[i];
        QFileInfo fi(fpath);
        QString filename = fi.fileName();

        int usedCount = 0;
        QStringList locations;

        for (const auto& pf : m_parsedProject) {
            if (pf.tree) {
                TSNode rootNode = ts_tree_root_node(pf.tree);
                std::function<void(TSNode)> searchIncludes = [&](TSNode node) {
                    const char* type = ts_node_type(node);
                    if (strcmp(type, "preproc_include") == 0 || strcmp(type, "import_statement") == 0 || strcmp(type, "import_from_statement") == 0) {
                        uint32_t startBytes = ts_node_start_byte(node);
                        uint32_t endBytes = ts_node_end_byte(node);
                        QString refText = QString::fromUtf8(pf.content.mid(startBytes, endBytes - startBytes));
                        if (refText.contains(filename)) {
                            usedCount++;
                            if (locations.size() < 3) {
                                int lineNo = pf.content.left(startBytes).count('\n') + 1;
                                locations.append(QFileInfo(pf.path).fileName() + ":" + QString::number(lineNo));
                            }
                        }
                    }
                    uint32_t childCount = ts_node_child_count(node);
                    for (uint32_t c = 0; c < childCount; ++c) { searchIncludes(ts_node_child(node, c)); }
                };
                searchIncludes(rootNode);
            } else {
                
                QString rawContent = QString::fromUtf8(pf.content);
                int index = 0;
                while ((index = rawContent.indexOf(filename, index)) != -1) {
                    if (index > 8 && (rawContent.mid(index - 9, 8).contains("include") || rawContent.mid(index - 7, 6).contains("import"))) {
                        usedCount++;
                        if (locations.size() < 3) {
                            int lineNo = rawContent.left(index).count('\n') + 1;
                            locations.append(QFileInfo(pf.path).fileName() + ":" + QString::number(lineNo));
                        }
                    }
                    index += filename.length();
                }
            }
        }

        FileResult r;
        r.file = filename;
        r.fullPath = fpath;
        r.type = fi.suffix();
        r.status = (usedCount == 0) ? "UNUSED" : "USED";
        r.count = usedCount;
        r.locations = locations.join(", ");
        m_fileResults.append(r);

        if (progressCallback) {
            progressCallback(80 + static_cast<int>((static_cast<double>(i) / totalFiles) * 20));
        }
    }

    return true;
}

void AnalysisEngine::runFallbackAnalysis(QVector<FunctionResult>& definitions) {
    QRegularExpression funcRegex(
        R"((?P<ret>(?:[\w:<>\*&\s]+))\s+(?P<name>(?:[\w:~]+::)*[\w:~]+)\s*\((?P<args>[^)]*)\)\s*(?:const|noexcept)?\s*\{)",
        QRegularExpression::MultilineOption
    );

    for (auto it = m_parsedProject.begin(); it != m_parsedProject.end(); ++it) {
        QString raw = QString::fromUtf8(it.value().content);
        QRegularExpressionMatchIterator matchIt = funcRegex.globalMatch(raw);
        while (matchIt.hasNext()) {
            QRegularExpressionMatch m = matchIt.next();
            QString fullName = m.captured("name").trimmed();
            QString shortName = fullName.split("::").last();

            if (shortName == "if" || shortName == "for" || shortName == "while" ||
                shortName == "switch" || shortName == "catch" || shortName == "else") { continue; }

            int lineNo = raw.left(m.capturedStart(0)).count('\n') + 1;
            FunctionResult res;
            res.signature = m.captured("ret").trimmed() + " " + fullName + "(" + m.captured("args").trimmed() + ")";
            res.file = it.key();
            res.line = lineNo;
            res.context = raw.mid(m.capturedStart(0), 150) + "\n... }";
            res.count = 0;
            definitions.append(res);
        }
    }
}

void AnalysisEngine::extractFunctionsFromAST(const ParsedFile& pf, QVector<FunctionResult>& definitions) {
    if (!pf.tree) return;
    TSNode rootNode = ts_tree_root_node(pf.tree);
    traverseAndFindFunctions(rootNode, pf, definitions);
}

void AnalysisEngine::traverseAndFindFunctions(TSNode node, const ParsedFile& pf, QVector<FunctionResult>& definitions) {
    const char* nodeType = ts_node_type(node);
    bool isFunc = false;

    const LanguageDefinition& def = LanguageRegistry::getDefinition(pf.lang);
    if (!def.functionNode.isEmpty() && strcmp(nodeType, def.functionNode.toUtf8().constData()) == 0) { isFunc = true; }

    if (isFunc) {
        uint32_t startByte = ts_node_start_byte(node);
        uint32_t endByte = ts_node_end_byte(node);
        
        int lineNo = pf.content.left(startByte).count('\n') + 1;
        QString entireBody = QString::fromUtf8(pf.content.mid(startByte, endByte - startByte));

        TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
        QString shortName = "unknown";
        if (!ts_node_is_null(nameNode)) {
            uint32_t ns = ts_node_start_byte(nameNode);
            uint32_t ne = ts_node_end_byte(nameNode);
            shortName = QString::fromUtf8(pf.content.mid(ns, ne - ns));
        }

        FunctionResult res;
        res.signature = (pf.lang == CodeLanguage::Python) ? ("def " + shortName + "()") : ("void " + shortName + "()");
        res.file = pf.path;
        res.line = lineNo;
        res.context = entireBody;
        res.count = 0;
        definitions.append(res);
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        traverseAndFindFunctions(ts_node_child(node, i), pf, definitions);
    }
}

void AnalysisEngine::countOccurrences(FunctionResult& def, const QVector<ParsedFile>& allFiles) {
    QString shortName = def.signature.section(' ', 1).section('(', 0, 0).trimmed();
    if (shortName.contains("::")) { shortName = shortName.split("::").last(); }

    QStringList locations;
    int count = 0;

    for (const auto& pf : allFiles) {
        if (pf.tree) {
            TSNode root = ts_tree_root_node(pf.tree);
            traverseAndCountCalls(root, pf.content, shortName, def.file, def.line, count, locations, QFileInfo(pf.path).fileName());
        } else {
            
            QString text = QString::fromUtf8(pf.content);
            QRegularExpression pattern(R"(\b)" + QRegularExpression::escape(shortName) + R"(\s*\()");
            QRegularExpressionMatchIterator fit = pattern.globalMatch(text);
            while (fit.hasNext()) {
                QRegularExpressionMatch m = fit.next();
                int mLine = text.left(m.capturedStart(0)).count('\n') + 1;
                if (!(QFileInfo(pf.path).fileName() == QFileInfo(def.file).fileName() && mLine == def.line)) {
                    count++;
                    if (locations.size() < 3) {
                        locations.append(QFileInfo(pf.path).fileName() + ":" + QString::number(mLine));
                    }
                }
            }
        }
    }

    def.count = count;
    def.locations = locations.join(", ");
}

void AnalysisEngine::traverseAndCountCalls(TSNode node, const QByteArray& source, const QString& shortName,
                                           const QString& defFile, int defLine, int& count,
                                           QStringList& locations, const QString& currentFileName) {
    const char* type = ts_node_type(node);
    if (strcmp(type, "call_expression") == 0) {
        TSNode functionNode = ts_node_child_by_field_name(node, "function", 8);
        if (!ts_node_is_null(functionNode)) {
            uint32_t sb = ts_node_start_byte(functionNode);
            uint32_t eb = ts_node_end_byte(functionNode);
            QString calledName = QString::fromUtf8(source.mid(sb, eb - sb)).trimmed();
            if (calledName.endsWith("." + shortName) || calledName.endsWith("->" + shortName) || calledName == shortName) {
                int lineNo = source.left(sb).count('\n') + 1;
                
                if (!(currentFileName == QFileInfo(defFile).fileName() && lineNo == defLine)) {
                    count++;
                    if (locations.size() < 3) {
                        locations.append(currentFileName + ":" + QString::number(lineNo));
                    }
                }
            }
        }
    }

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        traverseAndCountCalls(ts_node_child(node, i), source, shortName, defFile, defLine, count, locations, currentFileName);
    }
}
