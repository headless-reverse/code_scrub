#include "shrink.h"
#include "shrink_factory.h"

static QString shrinkLanguageId(ShrinkLanguage lang) {
    switch (lang) {
    case ShrinkLanguage::C: return "c";
    case ShrinkLanguage::Cpp: return "cpp";
    case ShrinkLanguage::Python: return "python";
    case ShrinkLanguage::Java: return "java";
    case ShrinkLanguage::JavaScript: return "javascript";
    case ShrinkLanguage::Html: return "html";
    case ShrinkLanguage::Css: return "css";
    }
    return "python";
}

static ShrinkLanguage shrinkLanguageFromId(const QString& id) {
    const CodeLanguage lang = codeLanguageFromId(id);
    switch (lang) {
    case CodeLanguage::C: return ShrinkLanguage::C;
    case CodeLanguage::Cpp: return ShrinkLanguage::Cpp;
    case CodeLanguage::Python: return ShrinkLanguage::Python;
    case CodeLanguage::Java: return ShrinkLanguage::Java;
    case CodeLanguage::JavaScript: return ShrinkLanguage::JavaScript;
    case CodeLanguage::Html: return ShrinkLanguage::Html;
    case CodeLanguage::Css: return ShrinkLanguage::Css;
    case CodeLanguage::Unknown: return ShrinkLanguage::Python;
    }
    return ShrinkLanguage::Python;
}

static CodeLanguage toCodeLanguage(ShrinkLanguage lang) {
    switch (lang) {
    case ShrinkLanguage::C: return CodeLanguage::C;
    case ShrinkLanguage::Cpp: return CodeLanguage::Cpp;
    case ShrinkLanguage::Python: return CodeLanguage::Python;
    case ShrinkLanguage::Java: return CodeLanguage::Java;
    case ShrinkLanguage::JavaScript: return CodeLanguage::JavaScript;
    case ShrinkLanguage::Html: return CodeLanguage::Html;
    case ShrinkLanguage::Css: return CodeLanguage::Css;
    }
    return CodeLanguage::Unknown;
}

void to_json(nlohmann::json& j, const ShrinkOptions& opts) {
    j = nlohmann::json{
        {"lang", shrinkLanguageId(opts.lang).toStdString()},
        {"remove_comments", opts.remove_comments},
        {"remove_blank", opts.remove_blank},
        {"collapse_blanks", opts.collapse_blanks},
        {"remove_docstrings", opts.remove_docstrings},
        {"strip_spaces", opts.strip_spaces},
        {"remove_type_hints", opts.remove_type_hints},
        {"join_multilines", opts.join_multilines},
        {"merge_imports", opts.merge_imports},
        {"inline_functions", opts.inline_functions},
        {"ultra_shrink", opts.ultra_shrink},
        {"obfuscate_locals", opts.obfuscate_locals},
        {"remove_pragmas", opts.remove_pragmas},
        {"remove_annotations", opts.remove_annotations},
        {"minify_imports", opts.minify_imports},
        {"minify_markup", opts.minify_markup},
        {"minify_css_selectors", opts.minify_css_selectors},
        {"mangle_js_variables", opts.mangle_js_variables},
        {"remove_unused_includes", opts.remove_unused_includes},
        {"remove_debug_logs", opts.remove_debug_logs},
        {"remove_line_directives", opts.remove_line_directives},
        {"remove_unused_imports", opts.remove_unused_imports},
        {"remove_pass", opts.remove_pass},
        {"remove_asserts", opts.remove_asserts},
        {"remove_prints", opts.remove_prints},
        {"concat_strings", opts.concat_strings},
        {"remove_final_modifiers", opts.remove_final_modifiers},
        {"remove_system_out", opts.remove_system_out},
        {"remove_nonessential_annotations", opts.remove_nonessential_annotations},
        {"remove_console", opts.remove_console},
        {"remove_debugger", opts.remove_debugger},
        {"strip_typescript", opts.strip_typescript},
        {"convert_arrow_functions", opts.convert_arrow_functions},
        {"object_shorthand", opts.object_shorthand},
        {"minify_booleans", opts.minify_booleans},
        {"minify_inline_assets", opts.minify_inline_assets},
        {"remove_default_attrs", opts.remove_default_attrs},
        {"unquote_attrs", opts.unquote_attrs},
        {"remove_optional_tags", opts.remove_optional_tags},
        {"minify_colors", opts.minify_colors},
        {"remove_zero_units", opts.remove_zero_units},
        {"css_shorthand", opts.css_shorthand},
        {"dedupe_css_rules", opts.dedupe_css_rules},
        {"remove_unused_functions", opts.remove_unused_functions},
        {"remove_unused_locals", opts.remove_unused_locals},
        {"ternary_converter", opts.ternary_converter},
        {"remove_empty_loops", opts.remove_empty_loops},
        {"optional_chaining", opts.optional_chaining},
        {"string_deduplication", opts.string_deduplication}
    };
}

void from_json(const nlohmann::json& j, ShrinkOptions& opts) {
    opts.lang = shrinkLanguageFromId(QString::fromStdString(j.value("lang", "python")));
    opts.remove_comments = j.value("remove_comments", true);
    opts.remove_blank = j.value("remove_blank", true);
    opts.collapse_blanks = j.value("collapse_blanks", true);
    opts.remove_docstrings = j.value("remove_docstrings", true);
    opts.strip_spaces = j.value("strip_spaces", true);
    opts.remove_type_hints = j.value("remove_type_hints", true);
    opts.join_multilines = j.value("join_multilines", true);
    opts.merge_imports = j.value("merge_imports", true);
    opts.inline_functions = j.value("inline_functions", true);
    opts.ultra_shrink = j.value("ultra_shrink", false);
    opts.obfuscate_locals = j.value("obfuscate_locals", false);
    opts.remove_pragmas = j.value("remove_pragmas", false);
    opts.remove_annotations = j.value("remove_annotations", false);
    opts.minify_imports = j.value("minify_imports", false);
    opts.minify_markup = j.value("minify_markup", false);
    opts.minify_css_selectors = j.value("minify_css_selectors", false);
    opts.mangle_js_variables = j.value("mangle_js_variables", false);
    opts.remove_unused_includes = j.value("remove_unused_includes", false);
    opts.remove_debug_logs = j.value("remove_debug_logs", false);
    opts.remove_line_directives = j.value("remove_line_directives", false);
    opts.remove_unused_imports = j.value("remove_unused_imports", false);
    opts.remove_pass = j.value("remove_pass", false);
    opts.remove_asserts = j.value("remove_asserts", false);
    opts.remove_prints = j.value("remove_prints", false);
    opts.concat_strings = j.value("concat_strings", false);
    opts.remove_final_modifiers = j.value("remove_final_modifiers", false);
    opts.remove_system_out = j.value("remove_system_out", false);
    opts.remove_nonessential_annotations = j.value("remove_nonessential_annotations", false);
    opts.remove_console = j.value("remove_console", false);
    opts.remove_debugger = j.value("remove_debugger", false);
    opts.strip_typescript = j.value("strip_typescript", false);
    opts.convert_arrow_functions = j.value("convert_arrow_functions", false);
    opts.object_shorthand = j.value("object_shorthand", false);
    opts.minify_booleans = j.value("minify_booleans", false);
    opts.minify_inline_assets = j.value("minify_inline_assets", false);
    opts.remove_default_attrs = j.value("remove_default_attrs", false);
    opts.unquote_attrs = j.value("unquote_attrs", false);
    opts.remove_optional_tags = j.value("remove_optional_tags", false);
    opts.minify_colors = j.value("minify_colors", false);
    opts.remove_zero_units = j.value("remove_zero_units", false);
    opts.css_shorthand = j.value("css_shorthand", false);
    opts.dedupe_css_rules = j.value("dedupe_css_rules", false);
    opts.remove_unused_functions = j.value("remove_unused_functions", false);
    opts.remove_unused_locals = j.value("remove_unused_locals", false);
    opts.ternary_converter = j.value("ternary_converter", false);
    opts.remove_empty_loops = j.value("remove_empty_loops", false);
    opts.optional_chaining = j.value("optional_chaining", false);
    opts.string_deduplication = j.value("string_deduplication", false);
}

QString shrinkCode(const QString& code, const ShrinkOptions& opts) {
    auto shrinker = ShrinkFactory::create(toCodeLanguage(opts.lang));
    return shrinker->shrink(code, opts);
}
