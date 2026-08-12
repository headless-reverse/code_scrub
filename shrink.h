#pragma once

#include <QString>
#include <QMap>
#include <QSet>
#include "language.h"
#include <nlohmann/json.hpp>

enum class ShrinkLanguage {
    C,
    Python,
    Cpp,
    Java,
    JavaScript,
    Html,
    Css
};

struct ShrinkOptions {
    ShrinkLanguage lang = ShrinkLanguage::Python;
    bool remove_comments = true;
    bool remove_blank = true;
    bool collapse_blanks = true;
    bool remove_docstrings = true;
    bool strip_spaces = true;
    bool remove_type_hints = true;
    bool join_multilines = true;
    bool merge_imports = true;
    bool inline_functions = true;
    bool ultra_shrink = false;
    bool obfuscate_locals = false;
    bool remove_pragmas = false;
    bool remove_annotations = false;
    bool minify_imports = false;
    bool minify_markup = false;
    bool minify_css_selectors = false;
    bool mangle_js_variables = false;
    bool remove_unused_includes = false;
    bool remove_debug_logs = false;
    bool remove_line_directives = false;
    bool remove_unused_imports = false;
    bool remove_pass = false;
    bool remove_asserts = false;
    bool remove_prints = false;
    bool concat_strings = false;
    bool remove_final_modifiers = false;
    bool remove_system_out = false;
    bool remove_nonessential_annotations = false;
    bool remove_console = false;
    bool remove_debugger = false;
    bool strip_typescript = false;
    bool convert_arrow_functions = false;
    bool object_shorthand = false;
    bool minify_booleans = false;
    bool minify_inline_assets = false;
    bool remove_default_attrs = false;
    bool unquote_attrs = false;
    bool remove_optional_tags = false;
    bool minify_colors = false;
    bool remove_zero_units = false;
    bool css_shorthand = false;
    bool dedupe_css_rules = false;
    bool remove_unused_functions = false;
    bool remove_unused_locals = false;
    bool ternary_converter = false;
    bool remove_empty_loops = false;
    bool optional_chaining = false;
    bool string_deduplication = false;
    QSet<int> excluded_lines;
    QSet<int> unused_function_lines;
};

void to_json(nlohmann::json& j, const ShrinkOptions& opts);
void from_json(const nlohmann::json& j, ShrinkOptions& opts);

QString shrinkCode(const QString& code, const ShrinkOptions& opts);
