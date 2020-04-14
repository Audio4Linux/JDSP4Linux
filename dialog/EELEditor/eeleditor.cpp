#include "eeleditor.h"
#include "ui_eeleditor.h"

#include "widgets/findreplaceform.h"

#include <QFile>
#include <QMap>
#include <QListWidgetItem>
#include <QSyntaxStyle.hpp>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>

EELEditor::EELEditor(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EELEditor)
{
    ui->setupUi(this);
    this->layout()->setMenuBar(ui->menuBar);

    ui->findReplaceForm->setTextEdit(ui->codeEdit);
    ui->findReplaceForm->hide();
    connect(ui->findReplaceForm,&FindReplaceForm::closedPressed,[this]{
        ui->findReplaceForm->hide();
    });

    completer = new EELCompleter();
    highlighter = new EELHighlighter();
    symbolProvider = new CustomSymbolProvider();

    symbolProvider->setLanguageSpecs(completer,highlighter);
    symbolProvider->setCodeEditorModule(ui->codeEdit);
    symbolProvider->setCodeOutlineModule(ui->outline);
    symbolProvider->connectSignals();

    ui->codeEdit->setCompleter(completer);
    ui->codeEdit->setHighlighter(highlighter);

    connect(ui->actionOpen_file,&QAction::triggered,[this]{
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open EEL script"), "", tr("EEL Script (*.eel)"));
        if(fileName.isEmpty())
            return;

        ui->projectView->addFile(fileName);
    });
    connect(ui->actionClose_filw,&QAction::triggered,[this]{
        ui->projectView->closeCurrentFile();
    });
    connect(ui->actionSave,&QAction::triggered,[this]{
        ui->projectView->getCurrentFile()->save();
        emit scriptSaved(ui->projectView->getCurrentFile()->path);
    });
    connect(ui->projectView,&ProjectView::currentFileUpdated,[this](CodeContainer* code){
        if(code == nullptr)
            ui->codeEdit->setText("//No code is loaded. Please create or open a file first.");
        else
            ui->codeEdit->loadCode(code);
        ui->codeEdit->setEnabled(code != nullptr);
    });

    ui->codeEdit->loadStyle(":/definitions/drakula.xml");

    symbolProvider->reloadSymbols();

    ui->splitter_main->setStretchFactor(0,1);
    ui->splitter_main->setStretchFactor(1,2);
    ui->splitter_sidebar->setStretchFactor(0,1);
    ui->splitter_sidebar->setStretchFactor(1,3);

    ui->codeEdit->setUndoRedoEnabled(true);
    connect(ui->actionUndo,&QAction::triggered,ui->codeEdit,&QCodeEditor::undo);
    connect(ui->actionRedo,&QAction::triggered,ui->codeEdit,&QCodeEditor::redo);
    connect(ui->actionFind_Replace,&QAction::triggered,[this]{
        ui->findReplaceForm->setVisible(!ui->findReplaceForm->isVisible());
    });
    connect(ui->actionGo_to_line,&QAction::triggered,[this]{
        bool ok;
        int line = QInputDialog::getInt(this, "Go to Line...",
                             "Enter line number:", ui->codeEdit->getCurrentLine()
                                        ,1,2147483647,1,&ok);
        if (ok && line >= 0)
            ui->codeEdit->goToLine(line);
    });
    connect(ui->actionGo_to_init,&QAction::triggered,[this]{
        ui->outline->goToAnnotation("@init");
    });
    connect(ui->actionGo_to_sample,&QAction::triggered,[this]{
        ui->outline->goToAnnotation("@sample");
    });
    connect(ui->actionJump_to_function,&QAction::triggered,[this]{
        bool ok;
        QString name = QInputDialog::getText(this, "Jump to Function...",
                             "Enter function name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty())
            ui->outline->goToFunction(name);
    });
}

EELEditor::~EELEditor()
{
    delete ui;
}

void EELEditor::openNewScript(QString path){
    ui->projectView->addFile(path);
}
