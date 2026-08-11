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
        {"mangle_js_variables", opts.mangle_js_variables}
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
}

QString shrinkCode(const QString& code, const ShrinkOptions& opts) {
    auto shrinker = ShrinkFactory::create(toCodeLanguage(opts.lang));
    return shrinker->shrink(code, opts);
}
