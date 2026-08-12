#pragma once

#include "shrink_base.h"

class WebShrinker : public IShrinker {
public:
    explicit WebShrinker(CodeLanguage language);
    QString shrink(const QString& code, const ShrinkOptions& opts) override;
    QStringList getSupportedOptions() const override;

private:
    CodeLanguage m_language;
};
