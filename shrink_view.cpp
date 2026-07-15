#include "shrink_view.h"
#include "shrink_parser.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QPainter>
#include <QTextBlock>
#include <QWheelEvent>

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
    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void LineNumberedTextEdit::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LineNumberedTextEdit::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void LineNumberedTextEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LineNumberedTextEdit::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn(1);
        } else if (event->angleDelta().y() < 0) {
            zoomOut(1);
        }
        updateLineNumberAreaWidth(0);
        event->accept();
    } else {
        QPlainTextEdit::wheelEvent(event);
    }
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
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                             Qt::AlignRight, number);
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

void LineNumberedTextEdit::clearHighlighting() {
    setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

ShrinkView::ShrinkView(QWidget *parent) : QWidget(parent) {
    initUi();
}

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

    master_check = new QCheckBox("Zaznacz wszystko", this);
    master_check->setChecked(true);

    side_layout->addWidget(btn_choose);
    side_layout->addWidget(file_path_edit);
    side_layout->addWidget(master_check);

    QMap<QString, QString> option_labels = {
        {"remove_comments", "Komentarze"},
        {"remove_blank", "Puste linie"},
        {"collapse_blanks", "Redukcja pustych linii"},
        {"remove_docstrings", "Docstringi (Python)"},
        {"strip_spaces", "Spacje końcowe"},
        {"remove_type_hints", "Adnotacje typów (Py)"},
        {"join_multilines", "Połącz łamane linie"},
        {"merge_imports", "Konsoliduj importy"},
        {"inline_functions", "Scal małe funkcje"},
        {"ultra_shrink", "Ultra Shrink"}
    };

    for (auto it = option_labels.begin(); it != option_labels.end(); ++it) {
        QCheckBox* cb = new QCheckBox(it.value(), this);
        cb->setChecked(it.key() != "ultra_shrink");
        option_checkboxes[it.key()] = cb;
        side_layout->addWidget(cb);
    }

    btn_shrink = new QPushButton("Shrink", this);
    btn_shrink->setStyleSheet("font-weight: bold;");
    btn_clear = new QPushButton("Wyczyść wynik", this);
    btn_save = new QPushButton("Zapisz", this);

    side_layout->addWidget(btn_shrink);
    side_layout->addWidget(btn_clear);
    side_layout->addWidget(btn_save);
    side_layout->addStretch();

    horizontal_splitter->addWidget(sidebar_widget);

    QSplitter* editors_splitter = new QSplitter(Qt::Vertical, this);
    src_edit = new LineNumberedTextEdit(this);
    src_edit->setPlaceholderText("// Wklej kod Python lub C/C++...");
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
    connect(master_check, &QCheckBox::clicked, this, &ShrinkView::toggleAllOptions);
    connect(btn_toggle_sidebar, &QPushButton::clicked, this, &ShrinkView::toggleSidebar);
}

void ShrinkView::toggleSidebar() {
    bool is_visible = sidebar_widget->isVisible();
    sidebar_widget->setVisible(!is_visible);
    if (!is_visible) {
        btn_toggle_sidebar->setText("◀ Ukryj panel opcji");
    } else {
        btn_toggle_sidebar->setText("▶ Pokaż panel opcji");
    }
}

void ShrinkView::toggleAllOptions(bool checked) {
    for (auto* cb : option_checkboxes) {
        cb->setChecked(checked);
    }
}

void ShrinkView::chooseFile() {
    QString path = QFileDialog::getOpenFileName(this, "Wybierz plik", "",
        "Pliki źródłowe (*.py *.cpp *.c *.h *.hpp *.cxx *.hxx);;Skrypty Pythona (*.py);;Pliki C/C++ (*.cpp *.c *.h *.hpp);;Wszystkie pliki (*.*)");
    if (!path.isEmpty()) {
        file_path_edit->setText(path);
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            src_edit->setPlainText(in.readAll());
            src_edit->clearHighlighting();
            dst_edit->clearHighlighting();
        }
    }
}

ShrinkOptions ShrinkView::getActiveOptions() const {
    ShrinkOptions opts;
    opts.remove_comments = option_checkboxes["remove_comments"]->isChecked();
    opts.remove_blank = option_checkboxes["remove_blank"]->isChecked();
    opts.collapse_blanks = option_checkboxes["collapse_blanks"]->isChecked();
    opts.remove_docstrings = option_checkboxes["remove_docstrings"]->isChecked();
    opts.strip_spaces = option_checkboxes["strip_spaces"]->isChecked();
    opts.remove_type_hints = option_checkboxes["remove_type_hints"]->isChecked();
    opts.join_multilines = option_checkboxes["join_multilines"]->isChecked();
    opts.merge_imports = option_checkboxes["merge_imports"]->isChecked();
    opts.inline_functions = option_checkboxes["inline_functions"]->isChecked();
    opts.ultra_shrink = option_checkboxes["ultra_shrink"]->isChecked();

    QString path = file_path_edit->text().toLower();
    if (path.endsWith(".cpp") || path.endsWith(".c") || path.endsWith(".h") ||
        path.endsWith(".hpp") || path.endsWith(".cxx") || path.endsWith(".hxx")) {
        opts.lang = ShrinkLanguage::Cpp;
    } else if (path.endsWith(".py")) {
        opts.lang = ShrinkLanguage::Python;
    } else {
        QString code = src_edit->toPlainText();
        if (code.contains("#include") || code.contains("using namespace") || (code.contains(";") && !code.contains("def "))) {
            opts.lang = ShrinkLanguage::Cpp;
        } else {
            opts.lang = ShrinkLanguage::Python;
        }
    }
    return opts;
}

void ShrinkView::doShrink() {
    QString raw_code = src_edit->toPlainText();
    if (raw_code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Obszar źródłowy nie może być pusty.");
        return;
    }
    QString result = shrinkCode(raw_code, getActiveOptions());
    dst_edit->setPlainText(result);
    
    updateDiffHighlighting();
}

void ShrinkView::updateDiffHighlighting() {
    QString originalText = src_edit->toPlainText();
    QString shrunkText = dst_edit->toPlainText();

    QStringList originalLines = originalText.split('\n');
    QStringList shrunkLines = shrunkText.split('\n');

    QSet<QString> originalTrimmedSet;
    for (const QString& line : originalLines) {
        originalTrimmedSet.insert(line.trimmed());
    }

    QSet<QString> shrunkTrimmedSet;
    for (const QString& line : shrunkLines) {
        shrunkTrimmedSet.insert(line.trimmed());
    }

    QSet<int> removedLines;
    for (int i = 0; i < originalLines.size(); ++i) {
        QString trimmed = originalLines[i].trimmed();
        if (!shrunkTrimmedSet.contains(trimmed)) {
            removedLines.insert(i);
        }
    }

    QSet<int> addedLines;
    for (int i = 0; i < shrunkLines.size(); ++i) {
        QString trimmed = shrunkLines[i].trimmed();
        if (!originalTrimmedSet.contains(trimmed)) {
            addedLines.insert(i);
        }
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
        target = QFileDialog::getSaveFileName(this, "Zapisz skurczony kod", "",
            "Skrypty Pythona (*.py);;Pliki C/C++ (*.cpp *.c *.h *.hpp);;Wszystkie pliki (*.*)");
        if (target.isEmpty()) return;
    }

    QFile file(target);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << data;
        file_path_edit->setText(target);
        QMessageBox::information(this, "Sukces", "Pomyślnie nadpisano plik:\n" + target);
    } else {
        QMessageBox::critical(this, "Błąd", "Nie można otworzyć pliku do zapisu:\n" + target);
    }
}
