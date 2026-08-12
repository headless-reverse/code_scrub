#include "shrink_java.h"
#include "formatter.h"
#include "shrink_parser.h"

QString JavaShrinker::shrink(const QString& code, const ShrinkOptions& opts) {
    ShrinkParser parser(code, CodeLanguage::Java);
    return CodeFormatter::format(parser.process(opts), opts, CodeLanguage::Java);
}

QStringList JavaShrinker::getSupportedOptions() const {
    return {
        "remove_comments",
        "remove_blank",
        "collapse_blanks",
        "strip_spaces",
        "merge_imports",
        "remove_docstrings",
        "remove_unused_imports",
        "remove_final_modifiers",
        "remove_system_out",
        "remove_annotations",
        "remove_nonessential_annotations",
        "minify_imports",
        "remove_unused_functions",
        "remove_unused_locals",
        "ternary_converter",
        "remove_empty_loops",
        "obfuscate_locals"
    };
}
