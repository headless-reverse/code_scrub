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
        "mangle_js_variables",
        "remove_console",
        "remove_debugger",
        "strip_typescript",
        "convert_arrow_functions",
        "object_shorthand",
        "minify_booleans",
        "minify_inline_assets",
        "remove_default_attrs",
        "unquote_attrs",
        "remove_optional_tags",
        "minify_colors",
        "remove_zero_units",
        "css_shorthand",
        "dedupe_css_rules",
        "remove_unused_functions",
        "remove_unused_locals",
        "ternary_converter",
        "remove_empty_loops",
        "optional_chaining",
        "string_deduplication"
    };
}
