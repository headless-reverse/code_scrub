#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct ClangDiagnostic {
    QString severity;
    QString text;
    int line;
    int column;
};

class ClangAnalyzer {
public:
    static QVector<ClangDiagnostic> analyze(const QString& filePath);
};
