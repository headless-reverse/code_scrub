#include "shrink_python.h"
#include "formatter.h"
#include "shrink_parser.h"

QString PythonShrinker::shrink(const QString& code, const ShrinkOptions& opts) {
    ShrinkParser parser(code, CodeLanguage::Python);
    return CodeFormatter::format(parser.process(opts), opts, CodeLanguage::Python);
}

QStringList PythonShrinker::getSupportedOptions() const {
    return {
        "remove_comments",
        "remove_blank",
        "collapse_blanks",
        "remove_docstrings",
        "strip_spaces",
        "remove_type_hints",
        "join_multilines",
        "merge_imports",
        "obfuscate_locals"
    };
}
