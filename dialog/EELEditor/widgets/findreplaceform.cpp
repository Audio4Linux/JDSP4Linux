/*
 * Copyright (C) 2009  Lorenzo Bettini <http://www.lorenzobettini.it>
 * See COPYING file that comes with this distribution
 */

#include <QtGui>
#include <QTextEdit>
#include <QRegExp>
#include <QSettings>
#include <QToolTip>

#include "findreplaceform.h"
#include "ui_findreplaceform.h"

#define TEXT_TO_FIND "textToFind"
#define TEXT_TO_REPLACE "textToReplace"
#define CASE_CHECK "caseCheck"
#define WHOLE_CHECK "wholeCheck"
#define REGEXP_CHECK "regexpCheck"

FindReplaceForm::FindReplaceForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindReplaceForm), textEdit(0)
{
    ui->setupUi(this);

    connect(ui->textToFind, SIGNAL(textChanged(QString)), this, SLOT(textToFindChanged()));
    connect(ui->textToFind, SIGNAL(textChanged(QString)), this, SLOT(validateRegExp(QString)));

    connect(ui->regexCheckBox, SIGNAL(toggled(bool)), this, SLOT(regexpSelected(bool)));

    connect(ui->findButton,  &QPushButton::clicked, [this]{
        findNext();
    });
    connect(ui->findPrevButton,  &QPushButton::clicked, [this]{
        findPrev();
    });
    connect(ui->closeButton, &QPushButton::clicked, this, &FindReplaceForm::closedPressed);

    connect(ui->replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(ui->replaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));
}

FindReplaceForm::~FindReplaceForm()
{
    delete ui;
}

void FindReplaceForm::hideReplaceWidgets() {
    ui->replaceLabel->setVisible(false);
    ui->textToReplace->setVisible(false);
    ui->replaceButton->setVisible(false);
    ui->replaceAllButton->setVisible(false);
}

void FindReplaceForm::setTextEdit(QTextEdit *textEdit_) {
    textEdit = textEdit_;
    connect(textEdit, SIGNAL(copyAvailable(bool)), ui->replaceButton, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), ui->replaceAllButton, SLOT(setEnabled(bool)));
}

void FindReplaceForm::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void FindReplaceForm::textToFindChanged() {
    ui->findButton->setEnabled(ui->textToFind->text().size() > 0);
    ui->findPrevButton->setEnabled(ui->textToFind->text().size() > 0);
}

void FindReplaceForm::regexpSelected(bool sel) {
    if (sel)
        validateRegExp(ui->textToFind->text());
    else
        validateRegExp("");
}

void FindReplaceForm::validateRegExp(const QString &text) {
    if (!ui->regexCheckBox->isChecked() || text.size() == 0) {
        return; // nothing to validate
    }

    QRegExp reg(text,
                (ui->caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));

    if (reg.isValid()) {
        showError("");
    } else {
        showError(reg.errorString());
    }
}

void FindReplaceForm::showError(const QString &error) {
    QToolTip::showText( this->mapToGlobal( QPoint( this->size().width() / 3, 0 ) ), error );
}

void FindReplaceForm::showMessage(const QString &message) {
    QToolTip::showText( this->mapToGlobal( QPoint( this->size().width() / 3, 0 ) ), message );
}

void FindReplaceForm::find(bool next) {
    if (!textEdit)
        return; // TODO: show some warning?

    // backward search
    bool back = !next;

    const QString &toSearch = ui->textToFind->text();

    bool result = false;

    QTextDocument::FindFlags flags;

    if (back)
        flags |= QTextDocument::FindBackward;
    if (ui->caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;

    if (ui->regexCheckBox->isChecked()) {
        QRegExp reg(toSearch,
                    (ui->caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive));

        qDebug() << "searching for regexp: " << reg.pattern();

        textCursor = textEdit->document()->find(reg, textCursor, flags);
        textEdit->setTextCursor(textCursor);
        result = (!textCursor.isNull());
    } else {
        qDebug() << "searching for: " << toSearch;

        result = textEdit->find(toSearch, flags);
    }

    if (result) {
        showError("");
    } else {
        showError(tr("no match found"));
        // move to the beginning of the document for the next find
        textCursor.setPosition(0);
        textEdit->setTextCursor(textCursor);
    }
}

void FindReplaceForm::replace() {
    if (!textEdit->textCursor().hasSelection()) {
        findNext();
    } else {
        textEdit->textCursor().insertText(ui->textToReplace->text());
        findNext();
    }
}

void FindReplaceForm::replaceAll() {
    int i=0;
    while (textEdit->textCursor().hasSelection()){
        textEdit->textCursor().insertText(ui->textToReplace->text());
        findNext();
        i++;
    }
    showMessage(tr("Replaced %1 occurrence(s)").arg(i));
}

void FindReplaceForm::writeSettings(QSettings &settings, const QString &prefix) {
    settings.beginGroup(prefix);
    settings.setValue(TEXT_TO_FIND, ui->textToFind->text());
    settings.setValue(TEXT_TO_REPLACE, ui->textToReplace->text());
    settings.setValue(CASE_CHECK, ui->caseCheckBox->isChecked());
    settings.setValue(WHOLE_CHECK, ui->wholeCheckBox->isChecked());
    settings.setValue(REGEXP_CHECK, ui->regexCheckBox->isChecked());
    settings.endGroup();
}

void FindReplaceForm::readSettings(QSettings &settings, const QString &prefix) {
    settings.beginGroup(prefix);
    ui->textToFind->setText(settings.value(TEXT_TO_FIND, "").toString());
    ui->textToReplace->setText(settings.value(TEXT_TO_REPLACE, "").toString());
    ui->caseCheckBox->setChecked(settings.value(CASE_CHECK, false).toBool());
    ui->wholeCheckBox->setChecked(settings.value(WHOLE_CHECK, false).toBool());
    ui->regexCheckBox->setChecked(settings.value(REGEXP_CHECK, false).toBool());
    settings.endGroup();
}
