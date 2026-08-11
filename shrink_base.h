#pragma once

#include <QString>
#include <QStringList>
#include "shrink.h"

class IShrinker {
public:
    virtual ~IShrinker() = default;
    virtual QString shrink(const QString& code, const ShrinkOptions& opts) = 0;
    virtual QStringList getSupportedOptions() const = 0;
};
