#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QBrush>
#include <QColor>
#include "check.h"

class FunctionAnalysisView : public QWidget {
    Q_OBJECT
public:
    explicit FunctionAnalysisView(QWidget* parent = nullptr);
    
    void clear();
    void addFunctionResult(const FunctionResult& res);
    void filter(const QString& text, const QString& statusFilter);
    QTreeWidget* treeWidget() const { return m_tree; }

signals:
    void itemClicked(QTreeWidgetItem* item);

private:
    QTreeWidget* m_tree;
};

class FileAnalysisView : public QWidget {
    Q_OBJECT
public:
    explicit FileAnalysisView(QWidget* parent = nullptr);
    
    void clear();
    void addFileResult(const FileResult& res);
    void filter(const QString& text, const QString& statusFilter);
    QTreeWidget* treeWidget() const { return m_tree; }

signals:
    void itemClicked(QTreeWidgetItem* item);

private:
    QTreeWidget* m_tree;
};
