#include "clang_analyzer.h"
#include <QDebug>

#ifdef HAS_LIBCLANG
#include <clang-c/Index.h>
#endif

QVector<ClangDiagnostic> ClangAnalyzer::analyze([[maybe_unused]] const QString& filePath) {
    QVector<ClangDiagnostic> diagnostics;

#ifdef HAS_LIBCLANG
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        filePath.toUtf8().constData(),
        nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_None
    );

    if (!unit) {
        clang_disposeIndex(index);
        return diagnostics;
    }

    unsigned numDiagnostics = clang_getNumDiagnostics(unit);
    for (unsigned i = 0; i < numDiagnostics; ++i) {
        CXDiagnostic diag = clang_getDiagnostic(unit, i);
        
        CXString stringDiag = clang_getDiagnosticSpelling(diag);
        QString text = QString::fromUtf8(clang_getCString(stringDiag));
        clang_disposeString(stringDiag);

        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
        QString severityStr = "INFO";
        if (severity == CXDiagnostic_Warning) severityStr = "WARNING";
        else if (severity >= CXDiagnostic_Error) severityStr = "ERROR";

        CXSourceLocation location = clang_getDiagnosticLocation(diag);
        unsigned line, column;
        clang_getSpellingLocation(location, nullptr, &line, &column, nullptr);

        ClangDiagnostic d;
        d.severity = severityStr;
        d.text = text;
        d.line = static_cast<int>(line);
        d.column = static_cast<int>(column);
        diagnostics.append(d);

        clang_disposeDiagnostic(diag);
    }

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
#endif

    return diagnostics;
}
