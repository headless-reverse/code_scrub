#pragma once

#include "shrink_base.h"

class JavaShrinker : public IShrinker {
public:
    QString shrink(const QString& code, const ShrinkOptions& opts) override;
    QStringList getSupportedOptions() const override;
};
