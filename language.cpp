#include "language.h"
#include <QFileInfo>

static const LanguageDefinition CppDef = {
    CodeLanguage::Cpp,
    "C++",
    {"cpp", "cc", "h", "hpp", "cxx", "hxx"},
    {"int", "float", "double", "char", "void", "class", "struct", "public", "private", "protected",
     "if", "else", "for", "while", "do", "return", "const", "static", "virtual", "using", "namespace", "include"},
    "function_definition",
    "class_specifier",
    "declaration",
    "parameter_declaration",
    "comment",
    "preproc_include"
};

static const LanguageDefinition CDef = {
    CodeLanguage::C,
    "C",
    {"c"},
    {"int", "float", "double", "char", "void", "struct", "enum", "typedef",
     "if", "else", "for", "while", "do", "return", "const", "static", "extern", "include"},
    "function_definition",
    "struct_specifier",
    "declaration",
    "parameter_declaration",
    "comment",
    "preproc_include"
};

static const LanguageDefinition PythonDef = {
    CodeLanguage::Python,
    "Python",
    {"py"},
    {"def", "class", "import", "from", "as", "if", "else", "elif", "for", "while", "return", "pass", "in", "is", "not", "and", "or"},
    "function_definition",
    "class_definition",
    "assignment",
    "identifier",
    "comment",
    "import_statement"
};

static const LanguageDefinition JavaDef = {
    CodeLanguage::Java,
    "Java",
    {"java"},
    {"class", "interface", "enum", "public", "private", "protected", "static", "final",
     "void", "int", "long", "double", "float", "boolean", "if", "else", "for", "while",
     "return", "new", "package", "import", "extends", "implements", "this", "super"},
    "method_declaration",
    "class_declaration",
    "local_variable_declaration",
    "formal_parameter",
    "comment",
    "import_declaration"
};

static const LanguageDefinition JavaScriptDef = {
    CodeLanguage::JavaScript,
    "JavaScript",
    {"js", "mjs", "cjs", "jsx"},
    {"function", "const", "let", "var", "class", "import", "export", "from", "return",
     "if", "else", "for", "while", "async", "await", "new", "this", "try", "catch"},
    "function_declaration",
    "class_declaration",
    "lexical_declaration",
    "formal_parameter",
    "comment",
    "import_statement"
};

static const LanguageDefinition HtmlDef = {
    CodeLanguage::Html,
    "HTML",
    {"html", "htm"},
    {"html", "head", "body", "script", "style", "div", "span", "section", "article", "main"},
    "element",
    "element",
    "attribute",
    "attribute",
    "comment",
    "script_element"
};

static const LanguageDefinition CssDef = {
    CodeLanguage::Css,
    "CSS",
    {"css"},
    {"@media", "@supports", "@keyframes", "display", "position", "color", "background", "margin", "padding"},
    "rule_set",
    "class_selector",
    "declaration",
    "property_name",
    "comment",
    "import_statement"
};

static const LanguageDefinition UnknownDef = {
    CodeLanguage::Unknown,
    "Unknown",
    {},
    {},
    "", "", "", "", "", ""
};

const LanguageDefinition& LanguageRegistry::getDefinition(CodeLanguage lang) {
    if (lang == CodeLanguage::C) return CDef;
    if (lang == CodeLanguage::Cpp) return CppDef;
    if (lang == CodeLanguage::Python) return PythonDef;
    if (lang == CodeLanguage::Java) return JavaDef;
    if (lang == CodeLanguage::JavaScript) return JavaScriptDef;
    if (lang == CodeLanguage::Html) return HtmlDef;
    if (lang == CodeLanguage::Css) return CssDef;
    return UnknownDef;
}

CodeLanguage LanguageRegistry::detectFromPath(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    if (CDef.extensions.contains(ext)) {
        return CodeLanguage::C;
    }
    if (CppDef.extensions.contains(ext)) {
        return CodeLanguage::Cpp;
    }
    if (PythonDef.extensions.contains(ext)) {
        return CodeLanguage::Python;
    }
    if (JavaDef.extensions.contains(ext)) {
        return CodeLanguage::Java;
    }
    if (JavaScriptDef.extensions.contains(ext)) {
        return CodeLanguage::JavaScript;
    }
    if (HtmlDef.extensions.contains(ext)) {
        return CodeLanguage::Html;
    }
    if (CssDef.extensions.contains(ext)) {
        return CodeLanguage::Css;
    }
    return CodeLanguage::Unknown;
}
