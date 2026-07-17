#include "language.h"
#include <QFileInfo>

static const LanguageDefinition CppDef = {
    CodeLanguage::Cpp,
    "C++",
    {"cpp", "c", "cc", "h", "hpp", "cxx", "hxx"},
    {"int", "float", "double", "char", "void", "class", "struct", "public", "private", "protected",
     "if", "else", "for", "while", "do", "return", "const", "static", "virtual", "using", "namespace", "include"},
    "function_definition",
    "class_specifier",
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

static const LanguageDefinition UnknownDef = {
    CodeLanguage::Unknown,
    "Unknown",
    {},
    {},
    "", "", "", "", "", ""
};

const LanguageDefinition& LanguageRegistry::getDefinition(CodeLanguage lang) {
    if (lang == CodeLanguage::Cpp) return CppDef;
    if (lang == CodeLanguage::Python) return PythonDef;
    return UnknownDef;
}

CodeLanguage LanguageRegistry::detectFromPath(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    if (CppDef.extensions.contains(ext)) {
        return CodeLanguage::Cpp;
    }
    if (PythonDef.extensions.contains(ext)) {
        return CodeLanguage::Python;
    }
    return CodeLanguage::Unknown;
}
