#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QMap>
#include <QSet>
#include "shrink.h"

class QCheckBox;
class QPushButton;
class QLineEdit;
class QSplitter;
class LineNumberArea;
class LineNumberedTextEdit;

class LineNumberedTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    LineNumberedTextEdit(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    
    void highlightLines(const QSet<int>& lineNumbers, const QColor& color);
    void clearHighlighting();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
private:
    LineNumberArea *lineNumberArea;
};

class LineNumberArea : public QWidget {
public:
    LineNumberArea(LineNumberedTextEdit *editor) : QWidget(editor), codeEditor(editor) {}
    QSize sizeHint() const override { return QSize(codeEditor->lineNumberAreaWidth(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { codeEditor->lineNumberAreaPaintEvent(event); }
private:
    LineNumberedTextEdit *codeEditor;
};

class ShrinkView : public QWidget {
    Q_OBJECT
public:
    explicit ShrinkView(QWidget *parent = nullptr);

private slots:
    void chooseFile();
    void doShrink();
    void clearOutput();
    void saveResult();
    void toggleAllOptions(bool checked);
    void toggleSidebar();
    void saveProfile();
    void loadProfile();
    void onScrollChanged(int val);

private:
    void initUi();
    ShrinkOptions getActiveOptions() const;
    void updateDiffHighlighting();

    QWidget* sidebar_widget;
    QPushButton* btn_toggle_sidebar;
    
    QLineEdit* file_path_edit;
    QCheckBox* master_check;
    QMap<QString, QCheckBox*> option_checkboxes;
    
    LineNumberedTextEdit* src_edit;
    LineNumberedTextEdit* dst_edit;

    QPushButton* btn_shrink;
    QPushButton* btn_clear;
    QPushButton* btn_save;
    QPushButton* btn_save_prof;
    QPushButton* btn_load_prof;
    
    bool m_isScrolling = false;
};
