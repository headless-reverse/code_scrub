#include "formatter.h"
#include <QStringList>
#include <QRegularExpression>
#include <QMap>

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

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        bool isImport = false;

        if (opts.merge_imports) {
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

    for (const QString& raw : lines) {
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

    if (opts.minify_imports && lang == CodeLanguage::Java) {
        processedText.replace(QRegularExpression(R"((?m)^import\s+java\.util\.\w+;\s*\nimport\s+java\.util\.\w+;\s*)"), "import java.util.*;\n");
    }

    if (opts.minify_markup && lang == CodeLanguage::Html) {
        processedText.replace(QRegularExpression(R"(>\s+<)"), "><");
        processedText.replace(QRegularExpression(R"(\s{2,})"), " ");
    }

    if (opts.minify_css_selectors && lang == CodeLanguage::Css) {
        processedText.replace(QRegularExpression(R"(\s*([{}:;,>+~])\s*)"), "\\1");
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
