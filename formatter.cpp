#include "formatter.h"
#include <QStringList>
#include <QRegularExpression>

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

    QStringList collectedImports;
    QStringList structuralLines;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        bool isImport = false;

        if (opts.merge_imports) {
            if (lang == CodeLanguage::Cpp && cppIncludeRegex.match(trimmed).hasMatch()) {
                isImport = true;
            } else if (lang == CodeLanguage::Python && pyImportRegex.match(trimmed).hasMatch()) { isImport = true; }
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
            if (lang == CodeLanguage::Cpp) {
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

    if (opts.ultra_shrink) {
        processedText.replace(QRegularExpression(R"(\s+)"), " ");
        if (lang == CodeLanguage::Cpp) {
            processedText.replace("; ", ";");
            processedText.replace(" { ", "{");
            processedText.replace(" } ", "}");
        }
    }

    return processedText;
}
