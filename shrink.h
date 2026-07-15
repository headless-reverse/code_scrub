#pragma once

#include <QString>
#include <QMap>

enum class ShrinkLanguage {
    Python,
    Cpp
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
};

QString shrinkCode(const QString& code, const ShrinkOptions& opts);
