#include "shrink_web.h"
#include "formatter.h"
#include "shrink_parser.h"

WebShrinker::WebShrinker(CodeLanguage language)
    : m_language(language) {}

QString WebShrinker::shrink(const QString& code, const ShrinkOptions& opts) {
    ShrinkParser parser(code, m_language);
    return CodeFormatter::format(parser.process(opts), opts, m_language);
}

QStringList WebShrinker::getSupportedOptions() const {
    return {
        "remove_comments",
        "remove_blank",
        "collapse_blanks",
        "strip_spaces",
        "minify_markup",
        "minify_css_selectors",
        "mangle_js_variables"
    };
}
