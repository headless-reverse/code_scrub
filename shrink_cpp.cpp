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
        "remove_pragmas",
        "obfuscate_locals"
    };
}
