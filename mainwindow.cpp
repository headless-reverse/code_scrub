#include "mainwindow.h"
#include "check_view.h"
#include "shrink_view.h"
#include "archive_worker.h"
#include "clang_analyzer.h"
#include "analysis_engine.h" 
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QFileDialog>
#include <QDirIterator>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QAction>
#include <QTreeWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_funcWorker(nullptr), m_fileWorker(nullptr) {
    initUi();
    resize(1400, 900);
    setWindowTitle("Code Scrub");

    QSettings settings("SoftwareHouse", "CodeScrub");
    if (settings.contains("geometry")) { restoreGeometry(settings.value("geometry").toByteArray()); }
    if (settings.contains("windowState")) { restoreState(settings.value("windowState").toByteArray()); }
}

MainWindow::~MainWindow() { cancelAnalysis(); }

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("SoftwareHouse", "CodeScrub");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::initUi() {
    preview = new ZoomableTextEdit(this);
    preview->setReadOnly(true);
    preview->setFont(QFont("Consolas", 11));
    setCentralWidget(preview);

    createDockWidgets();
    createMenuBar();
    createToolBar();
    createStatusBarWidget();
}

void MainWindow::createMenuBar() {
    menu_bar = menuBar();
    menu_files = menu_bar->addMenu("&Files");
    QAction* act_load = menu_files->addAction("Załaduj Projekt...");
    connect(act_load, &QAction::triggered, this, &MainWindow::loadProject);
    
    QAction* act_export = menu_files->addAction("Eksportuj CSV...");
    connect(act_export, &QAction::triggered, this, &MainWindow::exportCsv);

    QAction* act_archive = menu_files->addAction("Utwórz archiwum (.tar.gz)...");
    connect(act_archive, &QAction::triggered, this, &MainWindow::archiveProject);
    
    menu_files->addSeparator();
    QAction* act_exit = menu_files->addAction("Wyjście");
    connect(act_exit, &QAction::triggered, this, &QWidget::close);

    menu_process = menu_bar->addMenu("&Process");
    QAction* act_run_func = menu_process->addAction("Uruchom analizę funkcji");
    connect(act_run_func, &QAction::triggered, this, &MainWindow::runFunctionAnalysis);
    
    QAction* act_run_file = menu_process->addAction("Uruchom analizę plików");
    connect(act_run_file, &QAction::triggered, this, &MainWindow::runFileAnalysis);
    
    menu_process->addSeparator();
    QAction* act_cancel = menu_process->addAction("Anuluj aktualny proces");
    connect(act_cancel, &QAction::triggered, this, &MainWindow::cancelAnalysis);

    menu_settings = menu_bar->addMenu("&Settings");
    QAction* act_reset_layout = menu_settings->addAction("Resetuj układ okien");
    connect(act_reset_layout, &QAction::triggered, this, [this]() {
        if (QMessageBox::question(this, "Reset", "Przywrócić domyślny układ paneli?") == QMessageBox::Yes) {
            QSettings settings("SoftwareHouse", "CodeScrub");
            settings.remove("geometry");
            settings.remove("windowState");
            QMessageBox::information(this, "Reset", "Uruchom ponownie, aby zastosować zmiany.");
        }
    });

    menu_view = menu_bar->addMenu("&View");
    menu_view->addAction(dock_func->toggleViewAction());
    menu_view->addAction(dock_file->toggleViewAction());
    menu_view->addAction(dock_shrink->toggleViewAction());
    menu_view->addAction(dock_outline->toggleViewAction());
}

void MainWindow::createToolBar() {
    tool_bar = addToolBar("Narzędzia główne");
    tool_bar->setMovable(false);

    btn_load = new QPushButton("📁 Open...", this);
    search_box = new QLineEdit(this);
    search_box->setPlaceholderText("Filtruj...");
    search_box->setMaximumWidth(200);

    status_filter = new QComboBox(this);
    status_filter->addItems({"Wszystkie", "UNUSED", "DUPLICATE", "USED", "OK"});
    status_filter->setMaximumWidth(120);

    btn_run_func = new QPushButton("Funkcje", this);
    btn_run_file = new QPushButton("📂 Pliki", this);
    btn_cancel = new QPushButton("Anuluj", this);
    btn_cancel->setEnabled(false);
    btn_export = new QPushButton("💾 CSV", this);
    btn_archive = new QPushButton("📦 Archiwum", this);

    tool_bar->addWidget(btn_load);
    tool_bar->addSeparator();
    tool_bar->addWidget(new QLabel("Szukaj: ", this));
    tool_bar->addWidget(search_box);
    tool_bar->addWidget(status_filter);
    tool_bar->addSeparator();
    tool_bar->addWidget(btn_run_func);
    tool_bar->addWidget(btn_run_file);
    tool_bar->addWidget(btn_cancel);
    tool_bar->addSeparator();
    tool_bar->addWidget(btn_export);
    tool_bar->addWidget(btn_archive);

    connect(btn_load, &QPushButton::clicked, this, &MainWindow::loadProject);
    connect(btn_run_func, &QPushButton::clicked, this, &MainWindow::runFunctionAnalysis);
    connect(btn_run_file, &QPushButton::clicked, this, &MainWindow::runFileAnalysis);
    connect(btn_cancel, &QPushButton::clicked, this, &MainWindow::cancelAnalysis);
    connect(btn_export, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(btn_archive, &QPushButton::clicked, this, &MainWindow::archiveProject);
    connect(search_box, &QLineEdit::textChanged, this, &MainWindow::filterTrees);
    connect(status_filter, &QComboBox::currentTextChanged, this, &MainWindow::filterTrees);
}

void MainWindow::createDockWidgets() {
    dock_func = new QDockWidget("Analiza Funkcji", this);
    dock_func->setObjectName("dock_func");
    view_func = new FunctionAnalysisView(dock_func);
    dock_func->setWidget(view_func);
    addDockWidget(Qt::LeftDockWidgetArea, dock_func);

    dock_file = new QDockWidget("Analiza Plików", this);
    dock_file->setObjectName("dock_file");
    view_file = new FileAnalysisView(dock_file);
    dock_file->setWidget(view_file);
    addDockWidget(Qt::RightDockWidgetArea, dock_file);

    dock_shrink = new QDockWidget("Shrink", this);
    dock_shrink->setObjectName("dock_shrink");
    view_shrink = new ShrinkView(dock_shrink);
    dock_shrink->setWidget(view_shrink);
    addDockWidget(Qt::BottomDockWidgetArea, dock_shrink);

    dock_outline = new QDockWidget("Hierarchia AST (Outline)", this);
    dock_outline->setObjectName("dock_outline");
    view_outline = new QTreeWidget(dock_outline);
    view_outline->setHeaderLabels({"Struktura", "Typ elementu"});
    dock_outline->setWidget(view_outline);
    addDockWidget(Qt::RightDockWidgetArea, dock_outline);

    connect(view_func, &FunctionAnalysisView::itemClicked, this, &MainWindow::updatePreviewFromFunc);
    connect(view_file, &FileAnalysisView::itemClicked, this, &MainWindow::updatePreviewFromFile);
}

void MainWindow::createStatusBarWidget() {
    QStatusBar* status = statusBar();
    stats_label = new QLabel("Status: Gotowy", this);
    progress = new QProgressBar(this);
    progress->setMaximumWidth(200);
    progress->setValue(0);

    status->addWidget(stats_label, 1);
    status->addPermanentWidget(progress);
}

void MainWindow::loadProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "Katalog projektu C/C++ & Python");
    if (!dir.isEmpty()) {
        m_files.clear();
        QDirIterator it(dir, QStringList() << "*.cpp" << "*.c" << "*.cc" << "*.h" << "*.hpp" << "*.cxx" << "*.hxx" << "*.py", 
                        QDir::Files, QDirIterator::Subdirectories);
        
        while (it.hasNext()) {
            QString path = it.next();
            if (!path.contains("/build/") && !path.contains("/.git/") && !path.contains("/cmake-")) { m_files.append(path); }
        }
        stats_label->setText(QString("Załadowano %1 plików.").arg(m_files.size()));
    }
}

void MainWindow::runFunctionAnalysis() {
    if (m_files.isEmpty()) return;
    toggleButtons(false);
    view_func->clear();

    m_funcWorker = new FunctionAnalysisWorker(m_files, this);
    connect(m_funcWorker, &FunctionAnalysisWorker::progressUpdated, progress, &QProgressBar::setValue);
    connect(m_funcWorker, &FunctionAnalysisWorker::resultsReady, this, &MainWindow::onFunctionAnalysisFinished);
    connect(m_funcWorker, &FunctionAnalysisWorker::cancelled, this, &MainWindow::onAnalysisCancelled);
    m_funcWorker->start();
}

void MainWindow::onFunctionAnalysisFinished(const QVector<FunctionResult>& results, int total, int unused, int duplicates) {
    for (const auto& r : results) { view_func->addFunctionResult(r); }
    toggleButtons(true);
    stats_label->setText(QString("Metody: %1 | Nieużywane: %2 | Duplikaty: %3").arg(total).arg(unused).arg(duplicates));
}

void MainWindow::runFileAnalysis() {
    if (m_files.isEmpty()) return;
    toggleButtons(false);
    view_file->clear();

    m_fileWorker = new FileAnalysisWorker(m_files, this);
    connect(m_fileWorker, &FileAnalysisWorker::progressUpdated, progress, &QProgressBar::setValue);
    connect(m_fileWorker, &FileAnalysisWorker::resultsReady, this, &MainWindow::onFileAnalysisFinished);
    connect(m_fileWorker, &FileAnalysisWorker::cancelled, this, &MainWindow::onAnalysisCancelled);
    m_fileWorker->start();
}

void MainWindow::onFileAnalysisFinished(const QVector<FileResult>& results) {
    for (const auto& r : results) { view_file->addFileResult(r); }
    toggleButtons(true);
    stats_label->setText(QString("Pliki: %1 przetworzonych obiektów.").arg(results.size()));
}

void MainWindow::cancelAnalysis() {
    if (m_funcWorker && m_funcWorker->isRunning()) m_funcWorker->cancel();
    if (m_fileWorker && m_fileWorker->isRunning()) m_fileWorker->cancel();
}

void MainWindow::onAnalysisCancelled() {
    toggleButtons(true);
    stats_label->setText("Anulowano zadanie robocze.");
    progress->setValue(0);
}

void MainWindow::toggleButtons(bool enabled) {
    btn_load->setEnabled(enabled);
    btn_run_func->setEnabled(enabled);
    btn_run_file->setEnabled(enabled);
    btn_cancel->setEnabled(!enabled);
}

void MainWindow::updatePreviewFromFunc(QTreeWidgetItem* item) {
    if (item) {
        QString rawCode = item->data(0, Qt::UserRole).toString();
        preview->setPlainText(rawCode);

        CodeLanguage lang = LanguageRegistry::detectFromPath(item->text(1));
        if (m_highlighter) { delete m_highlighter; m_highlighter = nullptr; }
        m_highlighter = new AstHighlighter(preview->document(), lang);
    }
}

void MainWindow::updatePreviewFromFile(QTreeWidgetItem* item) {
    if (item) {
        QString path = item->text(5);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            preview->setPlainText(QString::fromUtf8(data));

            CodeLanguage lang = LanguageRegistry::detectFromPath(path);
            if (m_highlighter) { delete m_highlighter; m_highlighter = nullptr; }
            m_highlighter = new AstHighlighter(preview->document(), lang);

            buildCodeOutline(path);

            if (lang == CodeLanguage::Cpp) {
                QVector<ClangDiagnostic> diags = ClangAnalyzer::analyze(path);
                if (!diags.isEmpty()) {
                    QString info = QString("\n\n// --- [Ostrzeżenia kompilacji libclang dla pliku %1] ---").arg(QFileInfo(path).fileName());
                    for (const auto& d : diags) {
                        info += QString("\n// [%1] Linia %2, Kol %3: %4").arg(d.severity).arg(d.line).arg(d.column).arg(d.text);
                    }
                    preview->append(info);
                }
            }
        } else { preview->setPlainText("// Błąd ładowania pliku."); }
    }
}

void MainWindow::buildCodeOutline(const QString& filePath) {
    view_outline->clear();
    CodeLanguage lang = LanguageRegistry::detectFromPath(filePath);
    if (lang == CodeLanguage::Unknown) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray bytes = file.readAll();

    TSParser* parser = ts_parser_new();
    const TSLanguage* tsLang = TreeSitterLoader::getLanguage(lang);
    if (!tsLang || !parser) {
        if (parser) ts_parser_delete(parser);
        return;
    }
    ts_parser_set_language(parser, tsLang);
    TSTree* tree = ts_parser_parse_string(parser, nullptr, bytes.constData(), bytes.size());

    if (tree) {
        TSNode root = ts_tree_root_node(tree);
        const LanguageDefinition& def = LanguageRegistry::getDefinition(lang);

        std::function<void(TSNode, QTreeWidgetItem*)> parseNodes = [&](TSNode node, QTreeWidgetItem* parentItem) {
            const char* type = ts_node_type(node);
            QTreeWidgetItem* currentItem = parentItem;

            bool isClass = (strcmp(type, def.classNode.toUtf8().constData()) == 0);
            bool isFunc = (strcmp(type, def.functionNode.toUtf8().constData()) == 0);

            if (isClass || isFunc) {
                TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
                QString nodeName = "unknown";
                if (!ts_node_is_null(nameNode)) {
                    uint32_t ns = ts_node_start_byte(nameNode);
                    uint32_t ne = ts_node_end_byte(nameNode);
                    nodeName = QString::fromUtf8(bytes.mid(ns, ne - ns));
                }

                currentItem = new QTreeWidgetItem(parentItem ? parentItem : view_outline->invisibleRootItem());
                currentItem->setText(0, nodeName);
                currentItem->setText(1, isClass ? "Klasa" : "Funkcja");
                currentItem->setExpanded(true);
            }

            uint32_t childCount = ts_node_child_count(node);
            for (uint32_t i = 0; i < childCount; ++i) { parseNodes(ts_node_child(node, i), currentItem); }
        };

        parseNodes(root, nullptr);
        ts_tree_delete(tree);
    }
    ts_parser_delete(parser);
}

void MainWindow::filterTrees() {
    QString text = search_box->text().toLower();
    QString filter = status_filter->currentText();
    view_func->filter(text, filter);
    view_file->filter(text, filter);
}

void MainWindow::exportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this, "Zapisz CSV", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Sygnatura;Plik;Linia;Status;Uzycia;Gdzie uzyto\n";
        QTreeWidget* tree = view_func->treeWidget();
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = tree->topLevelItem(i);
            out << QString("%1;%2;%3;%4;%5;%6\n")
                   .arg(item->text(0))
                   .arg(item->text(1))
                   .arg(item->text(2))
                   .arg(item->text(3))
                   .arg(item->text(4))
                   .arg(item->text(5));
        }
    }
}

void MainWindow::archiveProject() {
    if (m_files.isEmpty()) {
        QMessageBox::warning(this, "Archiwum", "Brak plików do spakowania.");
        return;
    }

    QString archivePath = QFileDialog::getSaveFileName(this, "Zapisz skompresowane archiwum", "", "Archiwum TAR GZ (*.tar.gz)");
    if (archivePath.isEmpty()) return;

    progress->setValue(0);
    stats_label->setText("Kompresowanie plików...");

    ArchiveWorker* worker = new ArchiveWorker(m_files, archivePath, this);
    connect(worker, &ArchiveWorker::progressUpdated, this, &MainWindow::onArchiveProgress);
    connect(worker, &ArchiveWorker::finished, this, &MainWindow::onArchiveFinished);
    connect(worker, &ArchiveWorker::finished, worker, &ArchiveWorker::deleteLater);
    worker->start();
}

void MainWindow::onArchiveProgress(int percent) { progress->setValue(percent); }

void MainWindow::onArchiveFinished(bool success, const QString& msg) {
    progress->setValue(success ? 100 : 0);
    if (success) {
        QMessageBox::information(this, "Archiwum", "Projekt został skompresowany i zapisany.");
        stats_label->setText("Kompresowanie ukończone.");
    } else {
        QMessageBox::warning(this, "Archiwum", "Wystąpiły błędy: " + msg);
        stats_label->setText("Kompresowanie przerwane.");
    }
}
