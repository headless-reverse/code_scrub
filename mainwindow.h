#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QVector>
#include <QTreeWidgetItem>
#include <QCloseEvent>
#include <QSettings>
#include <QTextEdit>
#include <QWheelEvent>
#include "check.h"
#include "ast_highlighter.h"

class ZoomableTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit ZoomableTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->angleDelta().y() > 0) zoomIn(1);
            else if (event->angleDelta().y() < 0) zoomOut(1);
            event->accept();
        } else { QTextEdit::wheelEvent(event); }
    }
};

class QPushButton;
class QLineEdit;
class QComboBox;
class QLabel;
class QProgressBar;
class QDockWidget;
class QMenuBar;
class QMenu;
class QToolBar;
class QTreeWidget;

class FunctionAnalysisView;
class FileAnalysisView;
class ShrinkView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void loadProject();
    void runFunctionAnalysis();
    void runFileAnalysis();
    void cancelAnalysis();
    void exportCsv();
    
    void archiveProject();
    void onArchiveProgress(int percent);
    void onArchiveFinished(bool success, const QString& msg);

    void filterTrees();
    void updatePreviewFromFunc(QTreeWidgetItem* item);
    void updatePreviewFromFile(QTreeWidgetItem* item);
    
    void onFunctionAnalysisFinished(const QVector<FunctionResult>& results, int total, int unused, int duplicates);
    void onFileAnalysisFinished(const QVector<FileResult>& results);
    void onAnalysisCancelled();

private:
    void initUi();
    void createMenuBar();
    void createToolBar();
    void createDockWidgets();
    void createStatusBarWidget();
    void toggleButtons(bool enabled);
    
    void buildCodeOutline(const QString& filePath);

    QStringList m_files;
    FunctionAnalysisWorker* m_funcWorker;
    FileAnalysisWorker* m_fileWorker;

    ZoomableTextEdit* preview;
    AstHighlighter* m_highlighter = nullptr;

    QDockWidget* dock_func;
    QDockWidget* dock_file;
    QDockWidget* dock_shrink;
    QDockWidget* dock_outline;

    FunctionAnalysisView* view_func;
    FileAnalysisView* view_file;
    ShrinkView* view_shrink;
    QTreeWidget* view_outline;

    QMenuBar* menu_bar;
    QMenu* menu_files;
    QMenu* menu_process;
    QMenu* menu_settings;
    QMenu* menu_view;
    QToolBar* tool_bar;

    QPushButton* btn_load;
    QLineEdit* search_box;
    QComboBox* status_filter;
    QPushButton* btn_run_func;
    QPushButton* btn_run_file;
    QPushButton* btn_cancel;
    QPushButton* btn_export;
    QPushButton* btn_archive;

    QLabel* stats_label;
    QProgressBar* progress;
};
