#include <QString>
#include "shrink.h"
#include "analysis_engine.h"

class CodeFormatter {
public:
    static QString format(const QString& source, const ShrinkOptions& opts, CodeLanguage lang);
};
