#include "shrink.h"
#include "shrink_parser.h"
#include "formatter.h"

void to_json(nlohmann::json& j, const ShrinkOptions& opts) {
    j = nlohmann::json{
        {"lang", opts.lang == ShrinkLanguage::Cpp ? "cpp" : "python"},
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
        {"obfuscate_locals", opts.obfuscate_locals}
    };
}

void from_json(const nlohmann::json& j, ShrinkOptions& opts) {
    std::string l = j.value("lang", "python");
    opts.lang = (l == "cpp") ? ShrinkLanguage::Cpp : ShrinkLanguage::Python;
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
}

QString shrinkCode(const QString& code, const ShrinkOptions& opts) {
    CodeLanguage language = CodeLanguage::Unknown;
    if (opts.lang == ShrinkLanguage::Cpp) {
        language = CodeLanguage::Cpp;
    } else if (opts.lang == ShrinkLanguage::Python) { language = CodeLanguage::Python; }
    ShrinkParser parser(code, language);
    QString astProcessedCode = parser.process(opts);
    return CodeFormatter::format(astProcessedCode, opts, language);
}
