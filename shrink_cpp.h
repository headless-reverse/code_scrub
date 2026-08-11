#pragma once

#include "shrink_base.h"

class CppShrinker : public IShrinker {
public:
    explicit CppShrinker(CodeLanguage language);
    QString shrink(const QString& code, const ShrinkOptions& opts) override;
    QStringList getSupportedOptions() const override;

private:
    CodeLanguage m_language;
};
