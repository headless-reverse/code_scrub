#pragma once

#include <memory>
#include "shrink_base.h"

class ShrinkFactory {
public:
    static std::unique_ptr<IShrinker> create(CodeLanguage language);
};
