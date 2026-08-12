#pragma once

#include <QString>
#include <QStringList>
#include <QSet>
#include "language_defs.h"

struct LanguageDefinition {
    CodeLanguage langType;
    QString name;
    QStringList extensions;
    QSet<QString> keywords;
    
    QString functionNode;
    QString classNode;
    QString variableNode;
    QString parameterNode;
    QString commentNode;
    QString importNode;
};

class LanguageRegistry {
public:
    static const LanguageDefinition& getDefinition(CodeLanguage lang);
    static CodeLanguage detectFromPath(const QString& path);
};
