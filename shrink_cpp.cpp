#include "shrink_cpp.h"
#include "formatter.h"
#include "shrink_parser.h"

CppShrinker::CppShrinker(CodeLanguage language)
    : m_language(language == CodeLanguage::C ? CodeLanguage::C : CodeLanguage::Cpp) {}

QString CppShrinker::shrink(const QString& code, const ShrinkOptions& opts) {
    ShrinkParser parser(code, m_language);
    return CodeFormatter::format(parser.process(opts), opts, m_language);
}

QStringList CppShrinker::getSupportedOptions() const {
    return {
        "remove_comments",
        "remove_blank",
        "collapse_blanks",
        "strip_spaces",
        "join_multilines",
        "merge_imports",
        "inline_functions",
        "ultra_shrink",
        "remove_unused_includes",
        "remove_debug_logs",
        "remove_pragmas",
        "remove_line_directives",
        "remove_unused_functions",
        "remove_unused_locals",
        "ternary_converter",
        "remove_empty_loops",
        "string_deduplication",
        "obfuscate_locals"
    };
}
