#include "shrink.h"
#include "shrink_parser.h"
#include "formatter.h"

QString shrinkCode(const QString& code, const ShrinkOptions& opts) {
    CodeLanguage language = CodeLanguage::Unknown;
    if (opts.lang == ShrinkLanguage::Cpp) {
        language = CodeLanguage::Cpp;
    } else if (opts.lang == ShrinkLanguage::Python) {
        language = CodeLanguage::Python;
    }
    ShrinkParser parser(code, language);
    QString astProcessedCode = parser.process(opts);
    return CodeFormatter::format(astProcessedCode, opts, language);
}
