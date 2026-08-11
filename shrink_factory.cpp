#include "shrink_factory.h"
#include "shrink_cpp.h"
#include "shrink_java.h"
#include "shrink_python.h"
#include "shrink_web.h"

std::unique_ptr<IShrinker> ShrinkFactory::create(CodeLanguage language) {
    switch (language) {
    case CodeLanguage::C:
    case CodeLanguage::Cpp:
        return std::make_unique<CppShrinker>(language);
    case CodeLanguage::Python:
        return std::make_unique<PythonShrinker>();
    case CodeLanguage::Java:
        return std::make_unique<JavaShrinker>();
    case CodeLanguage::JavaScript:
    case CodeLanguage::Html:
    case CodeLanguage::Css:
        return std::make_unique<WebShrinker>(language);
    case CodeLanguage::Unknown:
        return std::make_unique<PythonShrinker>();
    }
    return std::make_unique<PythonShrinker>();
}
