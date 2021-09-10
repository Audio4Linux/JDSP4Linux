#include "eeleditor.h"
#include "ui_eeleditor.h"

#include "widgets/findreplaceform.h"
#include "widgets/projectview.h"
#include "widgets/consoleoutput.h"
#include "widgets/proxystyle.h"

#include <QFile>
#include <QMap>
#include <QListWidgetItem>
#include <QSyntaxStyle.hpp>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QDateTime>
#include <QMessageBox>

#include <DockAreaTitleBar.h>
#include <DockAreaTabBar.h>
#include <FloatingDockContainer.h>
#include <DockComponentsFactory.h>

using namespace ads;

EELEditor::EELEditor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EELEditor)
{
    this->setStyle(new ProxyStyle("Fusion"));

    QFontDatabase::addApplicationFont(":/fonts/CONSOLA.ttf");
    QFontDatabase::addApplicationFont(":/fonts/CONSOLAB.ttf");

    ui->setupUi(this);
    this->layout()->setMenuBar(ui->menuBar);

    CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
    CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);
    DockManager = new CDockManager(this);

    codeEdit = new CodeEditor(this);
    findReplaceForm = new FindReplaceForm(this);
    projectView = new ProjectView(this);
    codeOutline = new CodeOutline(this);
    consoleOutput = new ConsoleOutput(this);

    codeWidget = new QWidget(this);
    auto* codeLayout = new QVBoxLayout(codeWidget);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setMargin(0);
    codeLayout->addWidget(codeEdit);
    codeLayout->addWidget(findReplaceForm);

    CDockWidget* CentralDockWidget = new CDockWidget("CentralWidget");
    CentralDockWidget->setMinimumSize(0,0);
    CentralDockWidget->setWidget(codeWidget);
    auto* CentralDockArea = DockManager->setCentralWidget(CentralDockWidget);
    CentralDockWidget->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromContent);
    CentralDockArea->setAllowedAreas(DockWidgetArea::OuterDockAreas);


    CDockWidget* projectsDock = new CDockWidget("Loaded projects");
    projectsDock->setWidget(projectView);
    projectsDock->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidget);
    projectsDock->resize(250, 50);
    projectsDock->setMinimumSize(200,50);
    projectsDock->setGeometry(0,0,0,400);

    auto* leftArea = DockManager->addDockWidget(DockWidgetArea::LeftDockWidgetArea, projectsDock);
    ui->menuView->addAction(projectsDock->toggleViewAction());

    projectsDock = new CDockWidget("Code outline");
    projectsDock->setWidget(codeOutline);
    projectsDock->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidget);
    projectsDock->resize(250, 150);
    projectsDock->setMinimumSize(200,150);
    DockManager->addDockWidget(DockWidgetArea::BottomDockWidgetArea, projectsDock, leftArea);
    ui->menuView->addAction(projectsDock->toggleViewAction());

    auto* consoleDock = new CDockWidget("Console output");
    consoleDock->setWidget(consoleOutput);
    consoleDock->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidget);
    consoleDock->resize(250, 150);
    consoleDock->setMinimumSize(200,150);
    DockManager->addDockWidget(DockWidgetArea::BottomDockWidgetArea, consoleDock);
    ui->menuView->addAction(consoleDock->toggleViewAction());

    findReplaceForm->setTextEdit(codeEdit);
    findReplaceForm->hide();
    connect(findReplaceForm, &FindReplaceForm::closedPressed, findReplaceForm, &FindReplaceForm::hide);

    completer = new EELCompleter();
    highlighter = new EELHighlighter();
    symbolProvider = new CustomSymbolProvider();

    symbolProvider->setLanguageSpecs(completer,highlighter);
    symbolProvider->setCodeEditorModule(codeEdit);
    symbolProvider->setCodeOutlineModule(codeOutline);
    symbolProvider->connectSignals();

    codeEdit->setCompleter(completer);
    codeEdit->setHighlighter(highlighter);

    // Remove error highlighting if contents changed
    connect(codeEdit, &CodeEditor::backendRefreshRequired, [this]{
        if(ignoreErrorClearOnNextChangeEvent)
        {
            ignoreErrorClearOnNextChangeEvent = false;
            return;
        }

        highlighter->setErrorLine(-1);
        highlighter->rehighlight();
    });

    connect(ui->actionOpen_file,&QAction::triggered,[this]{
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open EEL script"), "", tr("EEL Script (*.eel)"));
        if(fileName.isEmpty())
            return;

        projectView->addFile(fileName);
    });
    connect(ui->actionClose_filw,&QAction::triggered,[this]{
        projectView->closeCurrentFile();
    });
    connect(ui->actionSave,&QAction::triggered,[this]{
        projectView->getCurrentFile()->code = codeEdit->toPlainText();
        projectView->getCurrentFile()->save();
        emit scriptSaved(projectView->getCurrentFile()->path);
    });
    connect(ui->actionSave_as,&QAction::triggered,[this]{
        QString path = projectView->getCurrentFile()->path;
        path = QFileDialog::getSaveFileName(this, "Save as",
                                                "", "EEL2 script (*.eel)");

        if (path.isEmpty())
            return;

        projectView->getCurrentFile()->code = codeEdit->toPlainText();
        projectView->getCurrentFile()->save(path);
        emit scriptSaved(path);
    });
    connect(projectView,&ProjectView::currentFileUpdated,[this](CodeContainer* prev, CodeContainer* code){
        ui->actionSave->setEnabled(code != nullptr);
        ui->actionSave_as->setEnabled(code != nullptr);
        ui->actionClose_filw->setEnabled(code != nullptr);
        ui->actionRun_code->setEnabled(code != nullptr);

        if(code == nullptr){
            codeEdit->setText("//No code is loaded. Please create or open a file first.");
        }
        else{
            codeEdit->loadCode(code);
        }
        codeEdit->setEnabled(code != nullptr);
    });

    symbolProvider->reloadSymbols();

    codeEdit->setUndoRedoEnabled(true);

    ui->actionUndo->setEnabled(false);
    ui->actionRedo->setEnabled(false);
    connect(codeEdit, &CodeEditor::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    connect(codeEdit, &CodeEditor::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect(ui->actionUndo,&QAction::triggered,codeEdit,&QCodeEditor::undo);
    connect(ui->actionRedo,&QAction::triggered,codeEdit,&QCodeEditor::redo);
    connect(ui->actionFind_Replace,&QAction::triggered,[this]{
        findReplaceForm->setVisible(!findReplaceForm->isVisible());
    });
    connect(ui->actionGo_to_line,&QAction::triggered,[this]{
        bool ok;
        int line = QInputDialog::getInt(this, "Go to Line...",
                             "Enter line number:", codeEdit->getCurrentLine()
                                        ,1,2147483647,1,&ok);
        if (ok && line >= 0)
            codeEdit->goToLine(line);
    });
    connect(ui->actionGo_to_init,&QAction::triggered,[this]{
        codeOutline->goToAnnotation("@init");
    });
    connect(ui->actionGo_to_sample,&QAction::triggered,[this]{
        codeOutline->goToAnnotation("@sample");
    });
    connect(ui->actionJump_to_function,&QAction::triggered,[this]{
        bool ok;
        QString name = QInputDialog::getText(this, "Jump to Function...",
                             "Enter function name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty())
            codeOutline->goToFunction(name);
    });
    connect(ui->actionEEL2_documentation, &QAction::triggered, []{
        QDesktopServices::openUrl(QUrl("https://github.com/james34602/EEL_VM"));
    });

    connect(ui->actionRun_code, &QAction::triggered, [this]{
        CodeContainer* cc = projectView->getCurrentFile();
        if(cc == nullptr)
        {
            QMessageBox::critical(this, "Cannot execute", "No script file opened. Please open one first and try again.");
            return;
        }

        // Save
        projectView->getCurrentFile()->code = codeEdit->toPlainText();
        projectView->getCurrentFile()->save();
        emit scriptSaved(projectView->getCurrentFile()->path);

        emit runCode(projectView->getCurrentFile()->path);
    });


    QFont font;
    font.setFamily("Consolas");
    font.setPointSize(10);
    codeEdit->setFont(font);

    changeSyntaxStyle(":/definitions/drakula.xml");
}

EELEditor::~EELEditor()
{
    delete ui;
}

void EELEditor::openNewScript(QString path){
    projectView->addFile(path);
}

void EELEditor::onCompilerStarted(const QString &scriptName)
{
    Q_UNUSED(scriptName)
    consoleOutput->clear();
    consoleOutput->printLowPriorityLine(QString("'%1' started compiling at %2").arg(QFileInfo(scriptName).fileName()).arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")));

}

void EELEditor::onCompilerFinished(int ret, const QString& retMsg, const QString& msg, const QString& scriptName, float initMs)
{

    int line = -1;

    if(ret <= 0)
    {
        QString message = msg;

        QRegularExpression linenumRe(R"(^(\d+):)");
        auto matchIterator = linenumRe.globalMatch(msg);
        if(matchIterator.hasNext()){
            auto match = matchIterator.next();
            bool ok;
            line = match.captured(1).toInt(&ok);
            if(!ok)
            {
                line = -1;
            }

            message = message.remove(0, match.capturedLength(0));
        }

        consoleOutput->clear();
        consoleOutput->printErrorLine(QString("%1 in '%2'<br>").arg(retMsg).arg(QFileInfo(scriptName).fileName()));

        auto findAnnotationLineExternal = [](const QString& path, const QString& name){
            QRegularExpression function(R"((^|(?<=\n))(\@[^\s\W]*))");
            int linenum = 1;

            QFile inputFile(path);
            if (inputFile.open(QIODevice::ReadOnly))
            {
               QTextStream in(&inputFile);
               while (!in.atEnd())
               {
                  QString line = in.readLine();
                  auto matchIterator = function.globalMatch(line);
                  if(matchIterator.hasNext()){

                      auto match = matchIterator.next();
                      if(line == name)
                      {
                          return linenum;
                      }
                  }
                  linenum++;
               }
               inputFile.close();
            }

            return -1;
        };

        int initLine = findAnnotationLineExternal(scriptName, "@init");
        int sampleLine = findAnnotationLineExternal(scriptName,"@sample");
        if(ret == -1) // error in @init
        {
            if(initLine >= 0)
                line += initLine;
            else
                line = -1;
        }
        else if(ret == -3) // error in @sample
        {
            if(sampleLine >= 0)
                line += sampleLine;
            else
                line = -1;
        }

        if(line >= 0)
        {
            consoleOutput->printErrorLine(QString("Line %1:%2").arg(line).arg(message));
        }
        else
        {
            consoleOutput->printErrorLine(msg);
        }

        consoleOutput->printLowPriorityLine(QString("<br>Compilation stopped at %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")));
    }
    else
    {
        consoleOutput->printLowPriorityLine(QString("Script initialization took %1ms").arg(initMs));
    }

    auto* cc = projectView->getCurrentFile();
    if(cc == nullptr)
    {
        return;
    }

    // Ddon't highlight if different script is opened in editor
    if(QFileInfo(scriptName) != QFileInfo(cc->path))
        return;

    if(line < 0)
        highlighter->setErrorLine(-1);
    else
        highlighter->setErrorLine(line - 1);

    ignoreErrorClearOnNextChangeEvent = true;
    codeEdit->updateStyle();
}

void EELEditor::onConsoleOutputReceived(const QString &buffer)
{
    consoleOutput->print(buffer);
}

void EELEditor::changeSyntaxStyle(QString def)
{
    // Workaround to solve stylesheet conflicts
    DockManager->setStyleSheet("");
    codeEdit->loadStyle(def);

    QString Result;
    QString FileName = ":ads/stylesheets/";
    FileName += CDockManager::testConfigFlag(CDockManager::FocusHighlighting)
        ? "focus_highlighting" : "default";
#ifdef Q_OS_LINUX
    FileName += "_linux";
#endif
    FileName += ".css";
    QFile StyleSheetFile(FileName);
    StyleSheetFile.open(QIODevice::ReadOnly);
    QTextStream StyleSheetStream(&StyleSheetFile);
    Result = StyleSheetStream.readAll();
    StyleSheetFile.close();
    DockManager->setStyleSheet(Result);
}
