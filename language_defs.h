#pragma once

#include <QString>

enum class CodeLanguage {
    C,
    Cpp,
    Python,
    Java,
    JavaScript,
    Html,
    Css,
    Unknown
};

inline QString codeLanguageId(CodeLanguage lang) {
    switch (lang) {
    case CodeLanguage::C: return "c";
    case CodeLanguage::Cpp: return "cpp";
    case CodeLanguage::Python: return "python";
    case CodeLanguage::Java: return "java";
    case CodeLanguage::JavaScript: return "javascript";
    case CodeLanguage::Html: return "html";
    case CodeLanguage::Css: return "css";
    case CodeLanguage::Unknown: return "unknown";
    }
    return "unknown";
}

inline CodeLanguage codeLanguageFromId(const QString& id) {
    const QString normalized = id.toLower();
    if (normalized == "c") return CodeLanguage::C;
    if (normalized == "cpp" || normalized == "c++") return CodeLanguage::Cpp;
    if (normalized == "python" || normalized == "py") return CodeLanguage::Python;
    if (normalized == "java") return CodeLanguage::Java;
    if (normalized == "javascript" || normalized == "js") return CodeLanguage::JavaScript;
    if (normalized == "html" || normalized == "htm") return CodeLanguage::Html;
    if (normalized == "css") return CodeLanguage::Css;
    return CodeLanguage::Unknown;
}
