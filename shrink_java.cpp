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
        "remove_annotations",
        "minify_imports",
        "obfuscate_locals"
    };
}
