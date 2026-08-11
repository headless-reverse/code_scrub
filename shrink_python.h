#pragma once

#include "shrink_base.h"

class PythonShrinker : public IShrinker {
public:
    QString shrink(const QString& code, const ShrinkOptions& opts) override;
    QStringList getSupportedOptions() const override;
};
