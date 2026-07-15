#include "check_view.h"
#include <QVBoxLayout>
#include <QHeaderView>

FunctionAnalysisView::FunctionAnalysisView(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(6);
    m_tree->setHeaderLabels({"Signature", "File", "Line", "Status", "Uses", "Locations"});
    
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemClicked, this, &FunctionAnalysisView::itemClicked);
}

void FunctionAnalysisView::clear() {
    m_tree->clear();
}

void FunctionAnalysisView::addFunctionResult(const FunctionResult& res) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
    item->setText(0, res.signature);
    item->setText(1, res.file);
    item->setText(2, QString::number(res.line));
    item->setText(3, res.status);
    item->setText(4, QString::number(res.count));
    item->setText(5, res.locations);
    
    item->setData(0, Qt::UserRole, res.context);

    if (res.status == "UNUSED") {
        item->setForeground(3, QBrush(QColor("#D32F2F")));
    } else if (res.status == "DUPLICATE") {
        item->setForeground(3, QBrush(QColor("#F57C00")));
    } else {
        item->setForeground(3, QBrush(QColor("#388E3C")));
    }
}

void FunctionAnalysisView::filter(const QString& text, const QString& statusFilter) {
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_tree->topLevelItem(i);
        bool matchesText = text.isEmpty() ||
                           item->text(0).toLower().contains(text) ||
                           item->text(1).toLower().contains(text);
        bool matchesStatus = (statusFilter == "Wszystkie") || (item->text(3) == statusFilter);
        item->setHidden(!(matchesText && matchesStatus));
    }
}

FileAnalysisView::FileAnalysisView(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(6);
    m_tree->setHeaderLabels({"File", "Type", "Status", "Uses", "Locations", "Full Path"});
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemClicked, this, &FileAnalysisView::itemClicked);
}

void FileAnalysisView::clear() {
    m_tree->clear();
}

void FileAnalysisView::addFileResult(const FileResult& res) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_tree);
    item->setText(0, res.file);
    item->setText(1, res.type);
    item->setText(2, res.status);
    item->setText(3, QString::number(res.count));
    item->setText(4, res.locations);
    item->setText(5, res.fullPath);

    if (res.status == "UNUSED") {
        item->setForeground(2, QBrush(QColor("#D32F2F")));
    } else {
        item->setForeground(2, QBrush(QColor("#388E3C")));
    }
}

void FileAnalysisView::filter(const QString& text, const QString& statusFilter) {
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_tree->topLevelItem(i);
        bool matchesText = text.isEmpty() ||
                           item->text(0).toLower().contains(text);
        bool matchesStatus = (statusFilter == "Wszystkie") || (item->text(2) == statusFilter);
        item->setHidden(!(matchesText && matchesStatus));
    }
}
