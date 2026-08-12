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
        "remove_unused_imports",
        "remove_pass",
        "remove_asserts",
        "remove_prints",
        "concat_strings",
        "remove_unused_functions",
        "remove_unused_locals",
        "ternary_converter",
        "remove_empty_loops",
        "string_deduplication",
        "obfuscate_locals"
    };
}
