#include "formatter.h"
#include <QStringList>
#include <QRegularExpression>
#include <QMap>
#include <QSet>

static QString removeUnusedCppIncludes(const QString& text) {
    QStringList lines = text.split('\n');
    QString body;
    for (const QString& line : lines) {
        if (!line.trimmed().startsWith("#include")) body += line + "\n";
    }

    QStringList kept;
    QRegularExpression includeRegex(R"(^\s*#include\s*[<"]([^>"]+)[>"])");
    for (const QString& line : lines) {
        QRegularExpressionMatch match = includeRegex.match(line);
        if (!match.hasMatch()) {
            kept.append(line);
            continue;
        }

        const QString header = match.captured(1);
        const QString stem = header.section('/', -1).section('.', 0, 0);
        const bool likelyUsed = header.endsWith(".h") || body.contains(QRegularExpression(R"(\b)" + QRegularExpression::escape(stem) + R"(\b)"));
        if (likelyUsed) kept.append(line);
    }
    return kept.join("\n");
}

static QString removeMatchingLines(const QString& text, const QRegularExpression& regex, const QSet<int>& excludedLines) {
    QStringList lines = text.split('\n');
    QStringList kept;
    for (int i = 0; i < lines.size(); ++i) {
        const int lineNo = i + 1;
        if (!excludedLines.contains(lineNo) && regex.match(lines[i]).hasMatch()) continue;
        kept.append(lines[i]);
    }
    return kept.join("\n");
}

static int countRegexMatches(const QString& text, const QRegularExpression& regex) {
    int count = 0;
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        count++;
    }
    return count;
}

static QString removeFunctionsByStartLine(const QString& text, CodeLanguage lang, const QSet<int>& startLines, const QSet<int>& excludedLines) {
    if (startLines.isEmpty()) return text;

    QStringList lines = text.split('\n');
    QSet<int> removeIndexes;
    for (int startLine : startLines) {
        if (startLine <= 0 || startLine > lines.size() || excludedLines.contains(startLine)) continue;
        const int startIndex = startLine - 1;
        int endIndex = startIndex;

        if (lang == CodeLanguage::Python) {
            const QString header = lines[startIndex];
            const int baseIndent = header.size() - header.trimmed().size();
            for (int i = startIndex + 1; i < lines.size(); ++i) {
                const QString trimmed = lines[i].trimmed();
                if (trimmed.isEmpty() || trimmed.startsWith("#")) {
                    endIndex = i;
                    continue;
                }
                const int indent = lines[i].size() - trimmed.size();
                if (indent <= baseIndent) break;
                endIndex = i;
            }
        } else {
            int depth = 0;
            bool started = false;
            for (int i = startIndex; i < lines.size(); ++i) {
                for (QChar ch : lines[i]) {
                    if (ch == '{') {
                        depth++;
                        started = true;
                    } else if (ch == '}') {
                        depth--;
                    }
                }
                endIndex = i;
                if (started && depth <= 0) break;
            }
        }

        for (int i = startIndex; i <= endIndex; ++i) {
            if (!excludedLines.contains(i + 1)) removeIndexes.insert(i);
        }
    }

    QStringList kept;
    for (int i = 0; i < lines.size(); ++i) {
        if (!removeIndexes.contains(i)) kept.append(lines[i]);
    }
    return kept.join("\n");
}

static QString removeUnusedImports(const QString& text, CodeLanguage lang) {
    QStringList lines = text.split('\n');
    QString body;
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (lang == CodeLanguage::Python && (trimmed.startsWith("import ") || trimmed.startsWith("from "))) continue;
        if (lang == CodeLanguage::Java && trimmed.startsWith("import ")) continue;
        body += line + "\n";
    }

    QStringList kept;
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (lang == CodeLanguage::Python && trimmed.startsWith("import ")) {
            QString module = trimmed.mid(7).section(" as ", 0, 0).section('.', 0, 0).section(',', 0, 0).trimmed();
            if (module.isEmpty() || body.contains(QRegularExpression(R"(\b)" + QRegularExpression::escape(module) + R"(\b)"))) kept.append(line);
            continue;
        }
        if (lang == CodeLanguage::Python && trimmed.startsWith("from ")) {
            QString name = trimmed.section(" import ", 1).section(" as ", 0, 0).section(',', 0, 0).trimmed();
            if (name == "*" || name.isEmpty() || body.contains(QRegularExpression(R"(\b)" + QRegularExpression::escape(name) + R"(\b)"))) kept.append(line);
            continue;
        }
        if (lang == CodeLanguage::Java && trimmed.startsWith("import ")) {
            QString className = trimmed;
            className.remove(QRegularExpression(R"(^import\s+static\s+)"));
            className.remove(QRegularExpression(R"(^import\s+)"));
            className.remove(';');
            className = className.section('.', -1).trimmed();
            if (className == "*" || body.contains(QRegularExpression(R"(\b)" + QRegularExpression::escape(className) + R"(\b)"))) kept.append(line);
            continue;
        }
        kept.append(line);
    }
    return kept.join("\n");
}

static QString concatLiteralStrings(QString text) {
    QRegularExpression concatRegex(R"((["'])([^"'\n]*)\1\s*\+\s*\1([^"'\n]*)\1)");
    bool changed = true;
    while (changed) {
        changed = false;
        QRegularExpressionMatch match = concatRegex.match(text);
        if (match.hasMatch()) {
            text.replace(match.capturedStart(0), match.capturedLength(0), match.captured(1) + match.captured(2) + match.captured(3) + match.captured(1));
            changed = true;
        }
    }
    return text;
}

static QString removeRedundantPythonPass(const QString& text) {
    QStringList lines = text.split('\n');
    QSet<int> removeLines;

    for (int i = 0; i < lines.size(); ++i) {
        const QString trimmed = lines[i].trimmed();
        if (trimmed != "pass" && !trimmed.startsWith("pass #")) continue;

        const int passIndent = lines[i].size() - lines[i].trimmed().size();
        bool hasSiblingStatement = false;
        for (int j = i - 1; j >= 0; --j) {
            const QString sibling = lines[j].trimmed();
            if (sibling.isEmpty() || sibling.startsWith("#")) continue;
            const int indent = lines[j].size() - lines[j].trimmed().size();
            if (indent < passIndent) break;
            if (indent == passIndent) {
                hasSiblingStatement = true;
                break;
            }
        }
        for (int j = i + 1; j < lines.size() && !hasSiblingStatement; ++j) {
            const QString sibling = lines[j].trimmed();
            if (sibling.isEmpty() || sibling.startsWith("#")) continue;
            const int indent = lines[j].size() - lines[j].trimmed().size();
            if (indent < passIndent) break;
            if (indent == passIndent) {
                hasSiblingStatement = true;
                break;
            }
        }

        if (hasSiblingStatement) removeLines.insert(i);
    }

    QStringList compacted;
    for (int i = 0; i < lines.size(); ++i) {
        if (!removeLines.contains(i)) compacted.append(lines[i]);
    }
    return compacted.join("\n");
}

static QString dedupeCssRules(const QString& text) {
    QString out;
    int cursor = 0;
    QSet<QString> seenBlocks;
    QRegularExpression ruleRegex(R"(([^{}]+)\{([^{}]*)\})");
    QRegularExpressionMatchIterator it = ruleRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        out += text.mid(cursor, match.capturedStart(0) - cursor);
        const QString body = match.captured(2).trimmed();
        if (!seenBlocks.contains(body)) {
            seenBlocks.insert(body);
            out += match.captured(0);
        }
        cursor = match.capturedEnd(0);
    }
    out += text.mid(cursor);
    return out;
}

static QString removeUnusedLocalAssignments(const QString& text, CodeLanguage lang, const QSet<int>& excludedLines) {
    QStringList lines = text.split('\n');
    QString all = text;
    QStringList kept;
    QRegularExpression pyAssign(R"(^\s*([A-Za-z_]\w*)\s*=\s*[^=].*)");
    QRegularExpression cAssign(R"(^\s*(?:auto|int|long|double|float|bool|char|QString|std::string|String|var|let|const)?\s+([A-Za-z_]\w*)\s*=\s*[^=].*;\s*$)");

    for (int i = 0; i < lines.size(); ++i) {
        if (excludedLines.contains(i + 1)) {
            kept.append(lines[i]);
            continue;
        }
        QRegularExpressionMatch match = (lang == CodeLanguage::Python) ? pyAssign.match(lines[i]) : cAssign.match(lines[i]);
        if (match.hasMatch()) {
            const QString name = match.captured(1);
            QRegularExpression refRegex(R"(\b)" + QRegularExpression::escape(name) + R"(\b)");
            if (countRegexMatches(all, refRegex) <= 1) continue;
        }
        kept.append(lines[i]);
    }
    return kept.join("\n");
}

static QString convertSimpleTernaries(QString text, CodeLanguage lang) {
    if (lang == CodeLanguage::Python) {
        text.replace(QRegularExpression(R"(if\s+([^:\n]+):\s*\n\s*([A-Za-z_]\w*)\s*=\s*([^\n]+)\n\s*else:\s*\n\s*\2\s*=\s*([^\n]+))"),
                     "\\2 = \\3 if \\1 else \\4");
        return text;
    }

    text.replace(QRegularExpression(R"(if\s*\(([^)]+)\)\s*\{\s*([A-Za-z_]\w*)\s*=\s*([^;{}]+);\s*\}\s*else\s*\{\s*\2\s*=\s*([^;{}]+);\s*\})"),
                 "\\2 = \\1 ? \\3 : \\4;");
    return text;
}

static QString removeEmptyLoops(QString text, CodeLanguage lang) {
    if (lang == CodeLanguage::Python) {
        text.remove(QRegularExpression(R"((?m)^\s*(?:for|while)\b[^\n]*:\s*\n\s*pass\s*(?:#.*)?\n?)"));
        return text;
    }
    text.remove(QRegularExpression(R"((?m)^\s*(?:for|while)\s*\([^)]*\)\s*\{\s*\}\s*;?\s*\n?)"));
    return text;
}

static QString applyOptionalChaining(QString text, CodeLanguage lang) {
    if (lang == CodeLanguage::JavaScript) {
        text.replace(QRegularExpression(R"(\b([A-Za-z_$][\w$]*)\s*&&\s*\1\.([A-Za-z_$][\w$]*)\s*&&\s*\1\.\2\.([A-Za-z_$][\w$]*))"), "\\1?.\\2?.\\3");
    }
    return text;
}

static QString deduplicateStrings(QString text, CodeLanguage lang) {
    if (lang == CodeLanguage::Html || lang == CodeLanguage::Css || lang == CodeLanguage::Java) return text;

    QMap<QString, int> counts;
    QRegularExpression literalRegex(R"((["'])([A-Za-z0-9_ .:/-]{8,})\1)");
    QRegularExpressionMatchIterator it = literalRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        counts[match.captured(0)]++;
    }

    QString prefix;
    int index = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() < 2) continue;
        const QString varName = QString("_s%1").arg(index++);
        if (lang == CodeLanguage::Python) prefix += varName + " = " + it.key() + "\n";
        else if (lang == CodeLanguage::JavaScript) prefix += "const " + varName + " = " + it.key() + ";\n";
        else prefix += "static const char* " + varName + " = " + it.key() + ";\n";
        text.replace(it.key(), varName);
    }

    return prefix.isEmpty() ? text : prefix + text;
}

static QString minifyInlineHtmlAssets(QString text, const ShrinkOptions& opts) {
    QRegularExpression styleRegex(R"(<style\b([^>]*)>([\s\S]*?)</style>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch styleMatch = styleRegex.match(text);
    while (styleMatch.hasMatch()) {
        QString css = styleMatch.captured(2);
        css.replace(QRegularExpression(R"(/\*[\s\S]*?\*/)"), "");
        css.replace(QRegularExpression(R"(\s*([{}:;,>+~])\s*)"), "\\1");
        if (opts.remove_zero_units) css.replace(QRegularExpression(R"(\b0(?:px|em|rem|pt|%)\b)"), "0");
        text.replace(styleMatch.capturedStart(0), styleMatch.capturedLength(0), "<style" + styleMatch.captured(1) + ">" + css.trimmed() + "</style>");
        styleMatch = styleRegex.match(text, styleMatch.capturedStart(0) + 7);
    }

    QRegularExpression scriptRegex(R"(<script\b([^>]*)>([\s\S]*?)</script>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch scriptMatch = scriptRegex.match(text);
    while (scriptMatch.hasMatch()) {
        QString js = scriptMatch.captured(2);
        js.replace(QRegularExpression(R"((?m)^\s*console\.(?:log|warn|error|debug|info)\([^;]*\);\s*\n?)"), "");
        js.replace(QRegularExpression(R"(\s+)"), " ");
        js.replace(QRegularExpression(R"(\s*([{}();,:=+\-*/<>])\s*)"), "\\1");
        text.replace(scriptMatch.capturedStart(0), scriptMatch.capturedLength(0), "<script" + scriptMatch.captured(1) + ">" + js.trimmed() + "</script>");
        scriptMatch = scriptRegex.match(text, scriptMatch.capturedStart(0) + 8);
    }
    return text;
}

QString CodeFormatter::format(const QString& source, const ShrinkOptions& opts, CodeLanguage lang) {
    QStringList lines = source.split(QRegularExpression(R"(\r?\n)"));
    
    if (opts.join_multilines) {
        QStringList joinedLines;
        QString buffer;
        for (const QString& line : lines) {
            if (line.endsWith('\\')) {
                QString temp = line;
                temp.chop(1);
                buffer += temp;
            } else {
                buffer += line;
                joinedLines.append(buffer);
                buffer.clear();
            }
        }
        if (!buffer.isEmpty()) { joinedLines.append(buffer); }
        lines = joinedLines;
    }

    QRegularExpression cppIncludeRegex(R"(^\s*#include\s*[<"].*[">])");
    QRegularExpression pyImportRegex(R"(^\s*(import\s+[\w\s,]+|from\s+[\w\.]+\s+import\s+[\w\s,\*]+))");
    QRegularExpression javaImportRegex(R"(^\s*import\s+(?:static\s+)?[\w.*]+;)");

    QStringList collectedImports;
    QStringList structuralLines;

    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const QString& line = lines[lineIndex];
        QString trimmed = line.trimmed();
        bool isImport = false;

        if (opts.merge_imports && !opts.excluded_lines.contains(lineIndex + 1)) {
            if ((lang == CodeLanguage::Cpp || lang == CodeLanguage::C) && cppIncludeRegex.match(trimmed).hasMatch()) {
                isImport = true;
            } else if (lang == CodeLanguage::Python && pyImportRegex.match(trimmed).hasMatch()) { isImport = true; }
            else if (lang == CodeLanguage::Java && javaImportRegex.match(trimmed).hasMatch()) { isImport = true; }
        }

        if (isImport) {
            if (!collectedImports.contains(trimmed)) { collectedImports.append(trimmed); }
        } else { structuralLines.append(line); }
    }
    lines = structuralLines;

    QStringList formatted;
    bool blankSeen = false;

    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const QString& raw = lines[lineIndex];
        if (opts.excluded_lines.contains(lineIndex + 1)) {
            formatted.append(raw);
            continue;
        }
        QString line = raw;
        QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) {
            if (opts.remove_blank) { continue; }
            if (opts.collapse_blanks) {
                if (blankSeen) continue;
                blankSeen = true;
            }
            formatted.append(line);
            continue;
        }
        
        blankSeen = false;
        
        if (opts.strip_spaces) {
            while (line.endsWith(' ') || line.endsWith('\t') || line.endsWith('\r')) { line.chop(1); }
        }

        formatted.append(line);
    }
    lines = formatted;

    if (opts.inline_functions) {
        QStringList inlinedLines;
        for (int i = 0; i < lines.size(); ++i) {
            QString current = lines[i];
            if (lang == CodeLanguage::Cpp || lang == CodeLanguage::C || lang == CodeLanguage::Java || lang == CodeLanguage::JavaScript) {
                if (current.trimmed().endsWith('{') && i + 2 < lines.size() && lines[i + 2].trimmed() == "}") {
                    QString body = lines[i + 1].trimmed();
                    if (body.length() < 60) {
                        current = current + " " + body + " }";
                        inlinedLines.append(current);
                        i += 2;
                        continue;
                    }
                }
            } else if (lang == CodeLanguage::Python) {
                if (current.trimmed().startsWith("def ") && current.trimmed().endsWith(":") && i + 1 < lines.size()) {
                    QString nextLine = lines[i + 1];
                    int indentCurrent = current.length() - current.trimmed().length();
                    int indentNext = nextLine.length() - nextLine.trimmed().length();
                    if (indentNext > indentCurrent) {
                        bool isShortFunc = false;
                        if (i + 2 >= lines.size()) {
                            isShortFunc = true;
                        } else {
                            QString lineAfter = lines[i + 2];
                            if (lineAfter.trimmed().isEmpty() || (lineAfter.length() - lineAfter.trimmed().length()) <= indentCurrent) { isShortFunc = true; }
                        }

                        if (isShortFunc && nextLine.trimmed().length() < 60) {
                            current = current + " " + nextLine.trimmed();
                            inlinedLines.append(current);
                            i += 1;
                            continue;
                        }
                    }
                }
            }
            inlinedLines.append(current);
        }
        lines = inlinedLines;
    }

    if (opts.merge_imports && !collectedImports.isEmpty()) {
        for (int i = collectedImports.size() - 1; i >= 0; --i) { lines.insert(0, collectedImports[i]); }
    }

    QString processedText = lines.join("\n");

    if (opts.remove_unused_functions) {
        processedText = removeFunctionsByStartLine(processedText, lang, opts.unused_function_lines, opts.excluded_lines);
    }

    if ((lang == CodeLanguage::Cpp || lang == CodeLanguage::C)) {
        if (opts.remove_unused_includes) processedText = removeUnusedCppIncludes(processedText);
        if (opts.remove_debug_logs) {
            processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*(?:std::(?:cout|cerr)\s*<<.*;|printf\s*\([^;]*\);|puts\s*\([^;]*\);|LOG_[A-Z_]*\s*\([^;]*\);)\s*$)"), opts.excluded_lines);
        }
        if (opts.remove_line_directives) {
            processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*#\s*line\b.*$)"), opts.excluded_lines);
        }
    }

    if (lang == CodeLanguage::Python) {
        if (opts.remove_unused_imports) processedText = removeUnusedImports(processedText, lang);
        if (opts.remove_pass) processedText = removeRedundantPythonPass(processedText);
        if (opts.remove_asserts) processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*assert\b.*$)"), opts.excluded_lines);
        if (opts.remove_prints) processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*print\s*\([^;\n]*\)\s*(?:#.*)?$)"), opts.excluded_lines);
        if (opts.concat_strings) processedText = concatLiteralStrings(processedText);
    }

    if (lang == CodeLanguage::Java) {
        if (opts.remove_unused_imports) processedText = removeUnusedImports(processedText, lang);
        if (opts.remove_final_modifiers) processedText.replace(QRegularExpression(R"(\bfinal\s+)"), "");
        if (opts.remove_system_out) processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*System\.(?:out|err)\.(?:print|println|printf)\s*\([^;]*\);\s*$)"), opts.excluded_lines);
        if (opts.remove_nonessential_annotations) {
            processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*@(Override|Deprecated|SuppressWarnings)(?:\([^)]*\))?\s*$)"), opts.excluded_lines);
        }
    }

    if (lang == CodeLanguage::JavaScript) {
        if (opts.remove_console) processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*console\.(?:log|warn|error|debug|info)\s*\([^;]*\);\s*$)"), opts.excluded_lines);
        if (opts.remove_debugger) processedText = removeMatchingLines(processedText, QRegularExpression(R"(^\s*debugger\s*;\s*$)"), opts.excluded_lines);
        if (opts.strip_typescript) {
            processedText.remove(QRegularExpression(R"((?m)^\s*interface\s+\w+\s*\{[\s\S]*?\}\s*)"));
            processedText.remove(QRegularExpression(R"((?m)^\s*type\s+\w+\s*=\s*[^;]+;\s*)"));
            processedText.remove(QRegularExpression(R"(:\s*[A-Za-z_$][\w$<>\[\]|&?, ]*(?=\s*[,)=;]))"));
        }
        if (opts.convert_arrow_functions) {
            processedText.replace(QRegularExpression(R"(function\s*\(([^)]*)\)\s*\{\s*return\s+([^;{}]+);\s*\})"), "(\\1)=>\\2");
        }
        if (opts.object_shorthand) {
            processedText.replace(QRegularExpression(R"(\b([A-Za-z_$][\w$]*)\s*:\s*\1\b)"), "\\1");
        }
        if (opts.minify_booleans) {
            processedText.replace(QRegularExpression(R"(\btrue\b)"), "!0");
            processedText.replace(QRegularExpression(R"(\bfalse\b)"), "!1");
        }
    }

    if (opts.remove_unused_locals) processedText = removeUnusedLocalAssignments(processedText, lang, opts.excluded_lines);
    if (opts.ternary_converter) processedText = convertSimpleTernaries(processedText, lang);
    if (opts.remove_empty_loops) processedText = removeEmptyLoops(processedText, lang);
    if (opts.optional_chaining) processedText = applyOptionalChaining(processedText, lang);
    if (opts.string_deduplication) processedText = deduplicateStrings(processedText, lang);

    if (opts.minify_imports && lang == CodeLanguage::Java) {
        processedText.replace(QRegularExpression(R"((?m)^import\s+java\.util\.\w+;\s*\nimport\s+java\.util\.\w+;\s*)"), "import java.util.*;\n");
    }

    if (opts.minify_markup && lang == CodeLanguage::Html) {
        processedText.replace(QRegularExpression(R"(>\s+<)"), "><");
        processedText.replace(QRegularExpression(R"(\s{2,})"), " ");
    }

    if (lang == CodeLanguage::Html) {
        if (opts.minify_inline_assets) processedText = minifyInlineHtmlAssets(processedText, opts);
        if (opts.remove_default_attrs) {
            processedText.remove(QRegularExpression(R"(\s+type=["']text/(?:javascript|css)["'])", QRegularExpression::CaseInsensitiveOption));
        }
        if (opts.unquote_attrs) {
            processedText.replace(QRegularExpression(R"((\s[\w:-]+)=["']([A-Za-z0-9_\-.:/#]+)["'])"), "\\1=\\2");
        }
        if (opts.remove_optional_tags) {
            processedText.remove(QRegularExpression(R"(</(?:p|li|td|tr)>)", QRegularExpression::CaseInsensitiveOption));
        }
    }

    if (opts.minify_css_selectors && lang == CodeLanguage::Css) {
        processedText.replace(QRegularExpression(R"(\s*([{}:;,>+~])\s*)"), "\\1");
    }

    if (lang == CodeLanguage::Css) {
        if (opts.minify_colors) {
            processedText.replace(QRegularExpression("#ffffff", QRegularExpression::CaseInsensitiveOption), "#fff");
            processedText.replace(QRegularExpression("#000000", QRegularExpression::CaseInsensitiveOption), "#000");
            processedText.replace(QRegularExpression("#ff0000", QRegularExpression::CaseInsensitiveOption), "red");
            processedText.replace(QRegularExpression(R"(rgb\s*\(\s*255\s*,\s*0\s*,\s*0\s*\))", QRegularExpression::CaseInsensitiveOption), "red");
        }
        if (opts.remove_zero_units) {
            processedText.replace(QRegularExpression(R"(\b0(?:px|em|rem|pt|%)\b)"), "0");
        }
        if (opts.css_shorthand) {
            processedText.replace(QRegularExpression(R"(margin-top:\s*([^;]+);\s*margin-right:\s*([^;]+);\s*margin-bottom:\s*\1;\s*margin-left:\s*\2;)"), "margin:\\1 \\2;");
            processedText.replace(QRegularExpression(R"(padding-top:\s*([^;]+);\s*padding-right:\s*([^;]+);\s*padding-bottom:\s*\1;\s*padding-left:\s*\2;)"), "padding:\\1 \\2;");
        }
        if (opts.dedupe_css_rules) processedText = dedupeCssRules(processedText);
    }

    if (opts.mangle_js_variables && lang == CodeLanguage::JavaScript) {
        QMap<QString, QString> renameMap;
        int counter = 0;
        QRegularExpression declRegex(R"(\b(?:let|const|var)\s+([A-Za-z_$][\w$]*))");
        QRegularExpressionMatchIterator it = declRegex.globalMatch(processedText);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const QString name = match.captured(1);
            if (!renameMap.contains(name)) {
                renameMap[name] = QString("v%1").arg(counter++);
            }
        }
        for (auto it = renameMap.constBegin(); it != renameMap.constEnd(); ++it) {
            processedText.replace(QRegularExpression(R"(\b)" + QRegularExpression::escape(it.key()) + R"(\b)"), it.value());
        }
    }

    if (opts.ultra_shrink) {
        processedText.replace(QRegularExpression(R"(\s+)"), " ");
        if (lang == CodeLanguage::Cpp || lang == CodeLanguage::C || lang == CodeLanguage::Java || lang == CodeLanguage::JavaScript || lang == CodeLanguage::Css) {
            processedText.replace("; ", ";");
            processedText.replace(" { ", "{");
            processedText.replace(" } ", "}");
        }
    }

    return processedText;
}
