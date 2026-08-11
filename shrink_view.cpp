#include "shrink_view.h"
#include "shrink_parser.h"
#include "ast_highlighter.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QStackedWidget>
#include <QVariant>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QPainter>
#include <QTextBlock>
#include <QWheelEvent>
#include <fstream>

LineNumberedTextEdit::LineNumberedTextEdit(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    connect(this, &LineNumberedTextEdit::blockCountChanged, this, &LineNumberedTextEdit::updateLineNumberAreaWidth);
    connect(this, &LineNumberedTextEdit::updateRequest, this, &LineNumberedTextEdit::updateLineNumberArea);
    updateLineNumberAreaWidth(0);
    setWordWrapMode(QTextOption::NoWrap);
}

int LineNumberedTextEdit::lineNumberAreaWidth() {
    int digits = 1;
    int max_blocks = qMax(1, blockCount());
    while (max_blocks >= 10) {
        max_blocks /= 10;
        digits++;
    }
    return 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void LineNumberedTextEdit::updateLineNumberAreaWidth(int) { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }

void LineNumberedTextEdit::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) { updateLineNumberAreaWidth(0); }
}

void LineNumberedTextEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LineNumberedTextEdit::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn(1);
        else if (event->angleDelta().y() < 0) zoomOut(1);
        updateLineNumberAreaWidth(0);
        event->accept();
    } else { QPlainTextEdit::wheelEvent(event); }
}

void LineNumberedTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#f0f0f0"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#787878"));
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}

void LineNumberedTextEdit::highlightLines(const QSet<int>& lineNumbers, const QColor& color) {
    QList<QTextEdit::ExtraSelection> selections;
    QTextDocument *doc = document();
    for (int line : lineNumbers) {
        QTextBlock block = doc->findBlockByNumber(line);
        if (block.isValid()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(color);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = QTextCursor(block);
            selections.append(selection);
        }
    }
    setExtraSelections(selections);
}

void LineNumberedTextEdit::clearHighlighting() { setExtraSelections(QList<QTextEdit::ExtraSelection>()); }

ShrinkView::ShrinkView(QWidget *parent) : QWidget(parent) { initUi(); }

void ShrinkView::initUi() {
    QVBoxLayout* top_level_layout = new QVBoxLayout(this);
    top_level_layout->setContentsMargins(4, 4, 4, 4);
    top_level_layout->setSpacing(4);

    QHBoxLayout* top_bar = new QHBoxLayout();
    btn_toggle_sidebar = new QPushButton("◀ Ukryj panel opcji", this);
    btn_toggle_sidebar->setFixedWidth(150);
    top_bar->addWidget(btn_toggle_sidebar);
    top_bar->addStretch();
    top_level_layout->addLayout(top_bar);

    QSplitter* horizontal_splitter = new QSplitter(Qt::Horizontal, this);

    sidebar_widget = new QWidget(this);
    sidebar_widget->setMinimumWidth(180);
    sidebar_widget->setMaximumWidth(300);
    QVBoxLayout* side_layout = new QVBoxLayout(sidebar_widget);
    side_layout->setContentsMargins(0, 0, 0, 0);
    side_layout->setSpacing(6);

    QPushButton* btn_choose = new QPushButton("Wybierz plik", this);
    file_path_edit = new QLineEdit(this);
    file_path_edit->setReadOnly(true);

    language_combo = new QComboBox(this);
    language_combo->addItem("C", QVariant::fromValue(static_cast<int>(CodeLanguage::C)));
    language_combo->addItem("C++", QVariant::fromValue(static_cast<int>(CodeLanguage::Cpp)));
    language_combo->addItem("Python", QVariant::fromValue(static_cast<int>(CodeLanguage::Python)));
    language_combo->addItem("Java", QVariant::fromValue(static_cast<int>(CodeLanguage::Java)));
    language_combo->addItem("HTML", QVariant::fromValue(static_cast<int>(CodeLanguage::Html)));
    language_combo->addItem("CSS", QVariant::fromValue(static_cast<int>(CodeLanguage::Css)));
    language_combo->addItem("JavaScript", QVariant::fromValue(static_cast<int>(CodeLanguage::JavaScript)));
    language_combo->setCurrentIndex(2);

    master_check = new QCheckBox("Zaznacz wszystko", this);
    master_check->setChecked(true);
    advanced_check = new QCheckBox("Pokaż opcje zaawansowane", this);
    advanced_check->setChecked(false);

    side_layout->addWidget(btn_choose);
    side_layout->addWidget(file_path_edit);
    side_layout->addWidget(new QLabel("Język", this));
    side_layout->addWidget(language_combo);
    side_layout->addWidget(master_check);
    side_layout->addWidget(advanced_check);

    auto addOption = [&](QVBoxLayout* layout, const QString& key, const QString& label, bool checked, bool advanced = false) {
        QCheckBox* cb = new QCheckBox(label, this);
        cb->setChecked(checked);
        option_checkboxes[key] = cb;
        if (advanced) {
            advanced_option_widgets[key] = cb;
            cb->setVisible(advanced_check->isChecked());
        }
        layout->addWidget(cb);
    };

    addOption(side_layout, "remove_comments", "Komentarze", true);
    addOption(side_layout, "remove_blank", "Puste linie", true);
    addOption(side_layout, "collapse_blanks", "Redukcja pustych linii", true);
    addOption(side_layout, "strip_spaces", "Spacje końcowe", true);

    language_options_stack = new QStackedWidget(this);

    QWidget* cpp_page = new QWidget(this);
    QVBoxLayout* cpp_layout = new QVBoxLayout(cpp_page);
    cpp_layout->setContentsMargins(0, 0, 0, 0);
    addOption(cpp_layout, "join_multilines", "Połącz łamane linie", true);
    addOption(cpp_layout, "merge_imports", "Konsoliduj include/import", true);
    addOption(cpp_layout, "inline_functions", "Scal małe funkcje", true, true);
    addOption(cpp_layout, "ultra_shrink", "Ultra Shrink", false, true);
    addOption(cpp_layout, "remove_pragmas", "Usuń #pragma", false, true);
    addOption(cpp_layout, "obfuscate_locals", "Zmień nazwy lokalne (AST)", false, true);
    cpp_layout->addStretch();

    QWidget* python_page = new QWidget(this);
    QVBoxLayout* python_layout = new QVBoxLayout(python_page);
    python_layout->setContentsMargins(0, 0, 0, 0);
    addOption(python_layout, "remove_docstrings", "Docstringi", true);
    addOption(python_layout, "remove_type_hints", "Adnotacje typów", true);
    addOption(python_layout, "py_join_multilines", "Połącz łamane linie", true);
    addOption(python_layout, "py_merge_imports", "Konsoliduj importy", true);
    addOption(python_layout, "py_obfuscate_locals", "Obfuskacja lokalna", false, true);
    python_layout->addStretch();

    QWidget* java_page = new QWidget(this);
    QVBoxLayout* java_layout = new QVBoxLayout(java_page);
    java_layout->setContentsMargins(0, 0, 0, 0);
    addOption(java_layout, "java_javadoc", "Javadoc", true);
    addOption(java_layout, "remove_annotations", "Usuń @Annotations", false, true);
    addOption(java_layout, "minify_imports", "Imports minification", false, true);
    addOption(java_layout, "java_obfuscate_locals", "Obfuskacja lokalna", false, true);
    java_layout->addStretch();

    QWidget* web_page = new QWidget(this);
    QVBoxLayout* web_layout = new QVBoxLayout(web_page);
    web_layout->setContentsMargins(0, 0, 0, 0);
    addOption(web_layout, "minify_markup", "Minifikacja znaczników", true);
    addOption(web_layout, "web_whitespace", "Usuń białe znaki", true);
    addOption(web_layout, "minify_css_selectors", "Kompresja selektorów CSS", false, true);
    addOption(web_layout, "mangle_js_variables", "Mangle zmiennych JS", false, true);
    web_layout->addStretch();

    language_options_stack->addWidget(cpp_page);
    language_options_stack->addWidget(python_page);
    language_options_stack->addWidget(java_page);
    language_options_stack->addWidget(web_page);
    side_layout->addWidget(language_options_stack);

    btn_shrink = new QPushButton("Shrink", this);
    btn_shrink->setStyleSheet("font-weight: bold;");
    btn_clear = new QPushButton("Wyczyść wynik", this);
    btn_save = new QPushButton("Zapisz", this);
    
    btn_save_prof = new QPushButton("Eksportuj Profil (JSON)", this);
    btn_load_prof = new QPushButton("Importuj Profil (JSON)", this);

    side_layout->addWidget(btn_shrink);
    side_layout->addWidget(btn_clear);
    side_layout->addWidget(btn_save);
    side_layout->addWidget(btn_save_prof);
    side_layout->addWidget(btn_load_prof);
    side_layout->addStretch();

    horizontal_splitter->addWidget(sidebar_widget);

    QSplitter* editors_splitter = new QSplitter(Qt::Vertical, this);
    src_edit = new LineNumberedTextEdit(this);
    src_edit->setPlaceholderText("// Wklej kod C, C++, Python, Java, HTML, CSS lub JavaScript...");
    dst_edit = new LineNumberedTextEdit(this);
    dst_edit->setPlaceholderText("// Wynik po minifikacji...");

    editors_splitter->addWidget(src_edit);
    editors_splitter->addWidget(dst_edit);
    
    horizontal_splitter->addWidget(editors_splitter);

    horizontal_splitter->setStretchFactor(0, 1);
    horizontal_splitter->setStretchFactor(1, 4);

    top_level_layout->addWidget(horizontal_splitter);

    connect(btn_choose, &QPushButton::clicked, this, &ShrinkView::chooseFile);
    connect(btn_shrink, &QPushButton::clicked, this, &ShrinkView::doShrink);
    connect(btn_clear, &QPushButton::clicked, this, &ShrinkView::clearOutput);
    connect(btn_save, &QPushButton::clicked, this, &ShrinkView::saveResult);
    connect(btn_save_prof, &QPushButton::clicked, this, &ShrinkView::saveProfile);
    connect(btn_load_prof, &QPushButton::clicked, this, &ShrinkView::loadProfile);
    connect(master_check, &QCheckBox::clicked, this, &ShrinkView::toggleAllOptions);
    connect(btn_toggle_sidebar, &QPushButton::clicked, this, &ShrinkView::toggleSidebar);
    connect(language_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ShrinkView::onLanguageChanged);
    connect(advanced_check, &QCheckBox::clicked, this, &ShrinkView::toggleAdvancedOptions);

    connect(src_edit->verticalScrollBar(), &QScrollBar::valueChanged, this, &ShrinkView::onScrollChanged);
    connect(dst_edit->verticalScrollBar(), &QScrollBar::valueChanged, this, &ShrinkView::onScrollChanged);
    updateOptionsPage(selectedLanguage());
    toggleAdvancedOptions(advanced_check->isChecked());
}

void ShrinkView::onScrollChanged(int val) {
    if (m_isScrolling) return;
    m_isScrolling = true;
    src_edit->verticalScrollBar()->setValue(val);
    dst_edit->verticalScrollBar()->setValue(val);
    m_isScrolling = false;
}

void ShrinkView::toggleSidebar() {
    bool is_visible = sidebar_widget->isVisible();
    sidebar_widget->setVisible(!is_visible);
    btn_toggle_sidebar->setText(!is_visible ? "◀ Ukryj panel opcji" : "▶ Pokaż panel opcji");
}

void ShrinkView::toggleAllOptions(bool checked) { for (auto* cb : option_checkboxes) cb->setChecked(checked); }

void ShrinkView::toggleAdvancedOptions(bool checked) {
    for (auto* widget : advanced_option_widgets) {
        widget->setVisible(checked);
    }
}

CodeLanguage ShrinkView::selectedLanguage() const {
    return static_cast<CodeLanguage>(language_combo->currentData().toInt());
}

void ShrinkView::setSelectedLanguage(CodeLanguage lang) {
    for (int i = 0; i < language_combo->count(); ++i) {
        if (static_cast<CodeLanguage>(language_combo->itemData(i).toInt()) == lang) {
            language_combo->setCurrentIndex(i);
            updateOptionsPage(lang);
            return;
        }
    }
}

void ShrinkView::updateOptionsPage(CodeLanguage lang) {
    int page = 1;
    if (lang == CodeLanguage::C || lang == CodeLanguage::Cpp) page = 0;
    else if (lang == CodeLanguage::Python) page = 1;
    else if (lang == CodeLanguage::Java) page = 2;
    else if (lang == CodeLanguage::Html || lang == CodeLanguage::Css || lang == CodeLanguage::JavaScript) page = 3;
    language_options_stack->setCurrentIndex(page);
}

void ShrinkView::onLanguageChanged(int) {
    updateOptionsPage(selectedLanguage());
}

void ShrinkView::chooseFile() {
    QString path = QFileDialog::getOpenFileName(this, "Wybierz plik", "",
        "Pliki źródłowe (*.py *.cpp *.c *.h *.hpp *.cxx *.hxx *.java *.js *.mjs *.html *.htm *.css);;Wszystkie pliki (*.*)");
    if (!path.isEmpty()) {
        file_path_edit->setText(path);
        CodeLanguage detected = LanguageRegistry::detectFromPath(path);
        if (detected != CodeLanguage::Unknown) {
            setSelectedLanguage(detected);
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            src_edit->setPlainText(file.readAll());
            src_edit->clearHighlighting();
            dst_edit->clearHighlighting();
        }
    }
}

ShrinkOptions ShrinkView::getActiveOptions() const {
    ShrinkOptions opts;
    auto checked = [&](const QString& key) {
        return option_checkboxes.contains(key) && option_checkboxes[key]->isChecked();
    };

    const CodeLanguage lang = selectedLanguage();
    opts.remove_comments = checked("remove_comments");
    opts.remove_blank = checked("remove_blank") || checked("web_whitespace");
    opts.collapse_blanks = checked("collapse_blanks");
    opts.remove_docstrings = checked("remove_docstrings") || checked("java_javadoc");
    opts.strip_spaces = checked("strip_spaces") || checked("web_whitespace");
    opts.remove_type_hints = checked("remove_type_hints");
    opts.join_multilines = checked("join_multilines") || checked("py_join_multilines");
    opts.merge_imports = checked("merge_imports") || checked("py_merge_imports");
    opts.inline_functions = checked("inline_functions");
    opts.ultra_shrink = checked("ultra_shrink");
    opts.obfuscate_locals = checked("obfuscate_locals") || checked("py_obfuscate_locals") || checked("java_obfuscate_locals");
    opts.remove_pragmas = checked("remove_pragmas");
    opts.remove_annotations = checked("remove_annotations");
    opts.minify_imports = checked("minify_imports");
    opts.minify_markup = checked("minify_markup");
    opts.minify_css_selectors = checked("minify_css_selectors");
    opts.mangle_js_variables = checked("mangle_js_variables");

    if (lang == CodeLanguage::C) opts.lang = ShrinkLanguage::C;
    else if (lang == CodeLanguage::Cpp) opts.lang = ShrinkLanguage::Cpp;
    else if (lang == CodeLanguage::Java) opts.lang = ShrinkLanguage::Java;
    else if (lang == CodeLanguage::Css) opts.lang = ShrinkLanguage::Css;
    else if (lang == CodeLanguage::JavaScript) opts.lang = ShrinkLanguage::JavaScript;
    else if (lang == CodeLanguage::Html) opts.lang = ShrinkLanguage::Html;
    else opts.lang = ShrinkLanguage::Python;
    return opts;
}

void ShrinkView::doShrink() {
    QString raw_code = src_edit->toPlainText();
    if (raw_code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Obszar źródłowy nie może być pusty.");
        return;
    }
    
    ShrinkOptions opts = getActiveOptions();
    QString result = shrinkCode(raw_code, opts);
    dst_edit->setPlainText(result);
    
    updateDiffHighlighting();
}

void ShrinkView::saveProfile() {
    QString path = QFileDialog::getSaveFileName(this, "Eksportuj Profil", "", "Profil JSON (*.json)");
    if (path.isEmpty()) return;

    nlohmann::json j;
    to_json(j, getActiveOptions());

    std::ofstream file(path.toStdString());
    if (file.is_open()) {
        file << j.dump(4);
        QMessageBox::information(this, "Profil", "Pomyślnie wyeksportowano profil konfiguracji.");
    }
}

void ShrinkView::loadProfile() {
    QString path = QFileDialog::getOpenFileName(this, "Importuj Profil", "", "Profil JSON (*.json)");
    if (path.isEmpty()) return;

    std::ifstream file(path.toStdString());
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        ShrinkOptions opts;
        from_json(j, opts);

        option_checkboxes["remove_comments"]->setChecked(opts.remove_comments);
        option_checkboxes["remove_blank"]->setChecked(opts.remove_blank);
        option_checkboxes["collapse_blanks"]->setChecked(opts.collapse_blanks);
        option_checkboxes["remove_docstrings"]->setChecked(opts.remove_docstrings);
        option_checkboxes["java_javadoc"]->setChecked(opts.remove_docstrings);
        option_checkboxes["strip_spaces"]->setChecked(opts.strip_spaces);
        option_checkboxes["remove_type_hints"]->setChecked(opts.remove_type_hints);
        option_checkboxes["join_multilines"]->setChecked(opts.join_multilines);
        option_checkboxes["py_join_multilines"]->setChecked(opts.join_multilines);
        option_checkboxes["merge_imports"]->setChecked(opts.merge_imports);
        option_checkboxes["py_merge_imports"]->setChecked(opts.merge_imports);
        option_checkboxes["inline_functions"]->setChecked(opts.inline_functions);
        option_checkboxes["ultra_shrink"]->setChecked(opts.ultra_shrink);
        option_checkboxes["obfuscate_locals"]->setChecked(opts.obfuscate_locals);
        option_checkboxes["py_obfuscate_locals"]->setChecked(opts.obfuscate_locals);
        option_checkboxes["java_obfuscate_locals"]->setChecked(opts.obfuscate_locals);
        option_checkboxes["remove_pragmas"]->setChecked(opts.remove_pragmas);
        option_checkboxes["remove_annotations"]->setChecked(opts.remove_annotations);
        option_checkboxes["minify_imports"]->setChecked(opts.minify_imports);
        option_checkboxes["minify_markup"]->setChecked(opts.minify_markup);
        option_checkboxes["minify_css_selectors"]->setChecked(opts.minify_css_selectors);
        option_checkboxes["mangle_js_variables"]->setChecked(opts.mangle_js_variables);
        if (opts.lang == ShrinkLanguage::C) setSelectedLanguage(CodeLanguage::C);
        else if (opts.lang == ShrinkLanguage::Cpp) setSelectedLanguage(CodeLanguage::Cpp);
        else if (opts.lang == ShrinkLanguage::Java) setSelectedLanguage(CodeLanguage::Java);
        else if (opts.lang == ShrinkLanguage::Css) setSelectedLanguage(CodeLanguage::Css);
        else if (opts.lang == ShrinkLanguage::JavaScript) setSelectedLanguage(CodeLanguage::JavaScript);
        else if (opts.lang == ShrinkLanguage::Html) setSelectedLanguage(CodeLanguage::Html);
        else setSelectedLanguage(CodeLanguage::Python);
    }
}

void ShrinkView::updateDiffHighlighting() {
    QStringList originalLines = src_edit->toPlainText().split('\n');
    QStringList shrunkLines = dst_edit->toPlainText().split('\n');

    QSet<QString> originalTrimmedSet, shrunkTrimmedSet;
    for (const QString& line : originalLines) originalTrimmedSet.insert(line.trimmed());
    for (const QString& line : shrunkLines) shrunkTrimmedSet.insert(line.trimmed());

    QSet<int> removedLines;
    for (int i = 0; i < originalLines.size(); ++i) {
        if (!shrunkTrimmedSet.contains(originalLines[i].trimmed())) removedLines.insert(i);
    }

    QSet<int> addedLines;
    for (int i = 0; i < shrunkLines.size(); ++i) {
        if (!originalTrimmedSet.contains(shrunkLines[i].trimmed())) addedLines.insert(i);
    }

    src_edit->highlightLines(removedLines, QColor("#ffe6e6"));
    dst_edit->highlightLines(addedLines, QColor("#e6ffe6"));
}

void ShrinkView::clearOutput() {
    dst_edit->clear();
    src_edit->clearHighlighting();
    dst_edit->clearHighlighting();
}

void ShrinkView::saveResult() {
    QString data = dst_edit->toPlainText();
    if (data.trimmed().isEmpty()) {
        QMessageBox::information(this, "Pusty wynik", "Brak danych do zapisu.");
        return;
    }
    QString target = file_path_edit->text();
    if (target.isEmpty()) {
        target = QFileDialog::getSaveFileName(this, "Zapisz", "", "Pliki źródłowe (*.py *.cpp *.c *.h *.hpp *.java *.js *.html *.css);;Wszystkie pliki (*.*)");
        if (target.isEmpty()) return;
    }
    QFile file(target);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << data;
        file_path_edit->setText(target);
        QMessageBox::information(this, "Sukces", "Zapisano plik.");
    }
}
