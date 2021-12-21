/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <vector>
#include <cmath>
#include <cfloat>
#include <QFileDialog>
#include <QScrollBar>
#include <QTextStream>
#include <QDebug>

#include "helpers/GainIterator.h"
#include "helpers/DPIHelper.h"
#include "GraphicEQFilterGUIView.h"
#include "GraphicEQFilterGUI.h"
#include "ui_GraphicEQFilterGUI.h"

using namespace std;

QRegularExpression GraphicEQFilterGUI::numberRegEx("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");

GraphicEQFilterGUI::GraphicEQFilterGUI(QWidget* parent) : QWidget(parent),
    ui(new Ui::GraphicEQFilterGUI)
{
    ui->setupUi(this);
    chk_enable = ui->chk_enable;
    
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(DPIHelper::scale(10));
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(DPIHelper::scale(10));
    ui->tableWidget->verticalHeader()->setMinimumSectionSize(DPIHelper::scale(23));
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(DPIHelper::scale(23));
    
    scene = new GraphicEQFilterGUIScene(ui->graphicsView);
    ui->graphicsView->setScene(scene);
    
    setPalette(qApp->style()->standardPalette());
    
    ui->toolBar->addAction(ui->actionInvertResponse);
    ui->toolBar->addAction(ui->actionNormalizeResponse);
    ui->toolBar->addAction(ui->actionResetResponse);
    ui->toolBar->addAction(ui->actionImport);
    ui->toolBar->addAction(ui->actionImport_AutoEQ_result);
    ui->toolBar->addAction(ui->actionExport);
    
    connect(scene, SIGNAL(nodeInserted(int,double,double)), this, SLOT(insertRow(int,double,double)));
    connect(scene, SIGNAL(nodeRemoved(int)), this, SLOT(removeRow(int)));
    connect(scene, SIGNAL(nodeUpdated(int,double,double)), this, SLOT(updateRow(int,double,double)));
    connect(scene, SIGNAL(nodeMoved(int,int)), this, SLOT(moveRow(int,int)));
    connect(scene, SIGNAL(nodeSelectionChanged(int,bool)), this, SLOT(selectRow(int,bool)));
    connect(scene, SIGNAL(updateModel(bool)), this, SIGNAL(updateModel(bool)));
    connect(scene, SIGNAL(nodeMouseUp(int)), this, SIGNAL(mouseUp()));

    connect(this, &GraphicEQFilterGUI::updateModel, this, [this](bool isMoving){
        if(!isMoving) emit updateModelEnd();
    });

    connect(ui->autoeq,&QAbstractButton::clicked,this,&GraphicEQFilterGUI::autoeqClicked);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

GraphicEQFilterGUI::~GraphicEQFilterGUI()
{
    delete ui;
}

void GraphicEQFilterGUI::setSidebarHidden(bool hidden){
    ui->sidebar->setVisible(!hidden);
}

void GraphicEQFilterGUI::set15BandFreeMode(bool e){
    freeMode15 = e;
    scene->setBandCount(15);
    setFreqEditable(true);
    scene->set15BandFreeMode(true);
}

void GraphicEQFilterGUI::store(QString& parameters)
{
    parameters = "GraphicEQ: ";
    
    bool first = true;
    for (FilterNode node : scene->getNodes())
    {
        if (first)
            first = false;
        else
            parameters += "; ";
        
        parameters += QString("%0 %1").arg(node.freq).arg(node.dbGain);
    }
}

void GraphicEQFilterGUI::storeCsv(QString& parameters)
{
    bool first = true;
    for (FilterNode node : scene->getNodes())
    {
        if (first)
            first = false;
        else
            parameters += ";";

        parameters += QString::number(node.freq);
    }
    for (FilterNode node : scene->getNodes())
    {
        parameters += ";";
        parameters += QString::number(node.dbGain);
    }
}

void GraphicEQFilterGUI::load(const QString& parameters)
{
    QStringList parameterList(parameters.split(":"));
    if(parameterList.count() < 2)
        return;

    std::vector<FilterNode> nodes;
    QStringList nodeList = parameterList.at(1).split(";");
    for(QString nodeStr : nodeList){
        QStringList values = nodeStr.trimmed().split(" ", Qt::SkipEmptyParts);
        if(values.count() != 2)
            continue;
        FilterNode node(values.at(0).toDouble(),values.at(1).toDouble());
        nodes.push_back(node);
    }

    sort(nodes.begin(), nodes.end());

    scene->setNodes(nodes);

    int bandCount = scene->verifyBands(nodes);
    if (bandCount != scene->getBandCount())
    {
        switch (bandCount)
        {
        case 15:
            ui->radioButton15->click();
            break;
        case 31:
            ui->radioButton31->click();
            break;
        default:
            ui->radioButtonVar->click();
        }
    }
    else
    {
        emit updateModel(false);
    }
}

void GraphicEQFilterGUI::loadMap(const QMap<float,float>& parameters)
{
    std::vector<FilterNode> nodes;
    for(auto p : parameters.toStdMap()){
        FilterNode node(p.first,p.second);
        nodes.push_back(node);
    }

    sort(nodes.begin(), nodes.end());

    scene->setNodes(nodes);

    int bandCount = scene->verifyBands(nodes);
    if (bandCount != scene->getBandCount())
    {
        switch (bandCount)
        {
        case 15:
            ui->radioButton15->click();
            break;
        case 31:
            ui->radioButton31->click();
            break;
        default:
            ui->radioButtonVar->click();
        }
    }
    else
    {
        emit updateModel(false);
    }
}

void GraphicEQFilterGUI::loadPreferences(const QVariantMap& prefs)
{
    /*ui->tableWidget->setFixedWidth(DPIHelper::scale(prefs.value("tableWidth", DEFAULT_TABLE_WIDTH).toDouble()));
    ui->graphicsView->setFixedHeight(DPIHelper::scale(prefs.value("viewHeight", DEFAULT_VIEW_HEIGHT).toDouble()));*/
    double zoomX = DPIHelper::scaleZoom(prefs.value("zoomX", 0.561).toDouble());
    double zoomY = DPIHelper::scaleZoom(prefs.value("zoomY", 3.773).toDouble());
    scene->setZoom(zoomX, zoomY);
    bool ok;
    int scrollX = DPIHelper::scale(prefs.value("scrollX", 102).toDouble(&ok));
    if (!ok)
        scrollX = round(scene->hzToX(20));
    int scrollY = DPIHelper::scale(prefs.value("scrollY", 825).toDouble(&ok));
    if (!ok)
        scrollY = round(scene->dbToY(22));
    ui->graphicsView->setScrollOffsets(scrollX, scrollY);
}

void GraphicEQFilterGUI::storePreferences(QVariantMap& prefs)
{
    /*if (DPIHelper::invScale(ui->tableWidget->width()) != DEFAULT_TABLE_WIDTH)
        prefs.insert("tableWidth", DPIHelper::invScale(ui->tableWidget->width()));
    if (DPIHelper::invScale(ui->graphicsView->height()) != DEFAULT_VIEW_HEIGHT)
        prefs.insert("viewHeight", DPIHelper::invScale(ui->graphicsView->height()));
    */if (DPIHelper::invScaleZoom(scene->getZoomX()) != 1.0)
        prefs.insert("zoomX", DPIHelper::invScaleZoom(scene->getZoomX()));
    if (DPIHelper::invScaleZoom(scene->getZoomY()) != 1.0)
        prefs.insert("zoomY", DPIHelper::invScaleZoom(scene->getZoomY()));
    QScrollBar* hScrollBar = ui->graphicsView->horizontalScrollBar();
    int value = hScrollBar->value();
    if (value != round(scene->hzToX(20)))
        prefs.insert("scrollX", DPIHelper::invScale(value));
    QScrollBar* vScrollBar = ui->graphicsView->verticalScrollBar();
    value = vScrollBar->value();
    if (value != round(scene->dbToY(22)))
        prefs.insert("scrollY", DPIHelper::invScale(value));
}

void GraphicEQFilterGUI::insertRow(int index, double hz, double db)
{
    ui->tableWidget->blockSignals(true);
    ui->tableWidget->insertRow(index);
    QTableWidgetItem* freqItem = new QTableWidgetItem(QString("%0").arg(hz));
    ui->tableWidget->setItem(index, 0, freqItem);
    
    QTableWidgetItem* gainItem = new QTableWidgetItem(QString("%0").arg(db));
    ui->tableWidget->setItem(index, 1, gainItem);
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::removeRow(int index)
{
    ui->tableWidget->blockSignals(true);
    ui->tableWidget->removeRow(index);
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::updateRow(int index, double hz, double db)
{
    ui->tableWidget->blockSignals(true);
    QTableWidgetItem* freqItem = ui->tableWidget->item(index, 0);
    freqItem->setText(QString("%0").arg(hz));
    
    QTableWidgetItem* gainItem = ui->tableWidget->item(index, 1);
    gainItem->setText(QString("%0").arg(db));
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::moveRow(int fromIndex, int toIndex)
{
    ui->tableWidget->blockSignals(true);
    QTableWidgetItem* freqItem = ui->tableWidget->takeItem(fromIndex, 0);
    QTableWidgetItem* gainItem = ui->tableWidget->takeItem(fromIndex, 1);
    bool selected = ui->tableWidget->selectionModel()->isRowSelected(fromIndex, ui->tableWidget->rootIndex());
    ui->tableWidget->removeRow(fromIndex);
    ui->tableWidget->insertRow(toIndex);
    ui->tableWidget->setItem(toIndex, 0, freqItem);
    ui->tableWidget->setItem(toIndex, 1, gainItem);
    if (selected)
    {
        QModelIndex modelIndex = ui->tableWidget->model()->index(toIndex, 0);
        ui->tableWidget->selectionModel()->select(modelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
        ui->tableWidget->scrollTo(modelIndex);
    }
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::selectRow(int index, bool select)
{
    ui->tableWidget->blockSignals(true);
    QItemSelectionModel::SelectionFlags command = QItemSelectionModel::Rows;
    if (select)
        command |= QItemSelectionModel::Select;
    else
        command |= QItemSelectionModel::Deselect;
    QModelIndex modelIndex = ui->tableWidget->model()->index(index, 0);
    ui->tableWidget->selectionModel()->select(modelIndex, command);
    ui->tableWidget->scrollTo(modelIndex);
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::on_tableWidget_cellChanged(int row, int column)
{
    if (column == 0)
    {
        QTableWidgetItem* freqItem = ui->tableWidget->item(row, 0);
        bool ok;
        double freq = freqItem->text().toDouble(&ok);
        if (ok)
        {
            double gain = scene->getNodes()[row].dbGain;
            scene->setNode(row, freq, gain);
        }
        else
        {
            freq = scene->getNodes()[row].freq;
            freqItem->setText(QString("%0").arg(freq));
        }
    }
    else if (column == 1)
    {
        QTableWidgetItem* gainItem = ui->tableWidget->item(row, 1);
        bool ok;
        double gain = gainItem->text().toDouble(&ok);
        if (ok)
        {
            double freq = scene->getNodes()[row].freq;
            scene->setNode(row, freq, gain);
        }
        else
        {
            gain = scene->getNodes()[row].dbGain;
            gainItem->setText(QString("%0").arg(gain));
        }
    }
}

void GraphicEQFilterGUI::on_tableWidget_itemSelectionChanged()
{
    QSet<int> selectedRows;
    for (QTableWidgetItem* item : ui->tableWidget->selectedItems())
        selectedRows.insert(item->row());
    
    scene->setSelectedNodes(selectedRows);
}

void GraphicEQFilterGUI::on_radioButton15_toggled(bool checked)
{
    if (checked && !freeMode15)
    {
        scene->setBandCount(15);
        setFreqEditable(false);
    }
}

void GraphicEQFilterGUI::on_radioButton31_toggled(bool checked)
{
    if (checked && !freeMode15)
    {
        scene->setBandCount(31);
        setFreqEditable(false);
    }
}

void GraphicEQFilterGUI::on_radioButtonVar_toggled(bool checked)
{
    if (checked && !freeMode15)
    {
        scene->setBandCount(-1);
        setFreqEditable(true);
    }
}

void GraphicEQFilterGUI::setFreqEditable(bool editable)
{
    ui->tableWidget->blockSignals(true);
    for (int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        QTableWidgetItem* item = ui->tableWidget->item(i, 0);
        if (editable)
            item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        else
            item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled));
    }
    ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::on_actionImport_triggered()
{
    QFileInfo fileInfo(configPath);
    QFileDialog dialog(this, tr("Import frequency response"), fileInfo.absolutePath(), "*.csv");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList nameFilters;
    nameFilters.append(tr("CSV frequency response (*.csv)"));
    nameFilters.append(tr("All files (*.*)"));
    dialog.setNameFilters(nameFilters);
    if (dialog.exec() == QDialog::Accepted)
    {
        vector<FilterNode> newNodes;
        
        for (QString path : dialog.selectedFiles())
        {
            QFile file(path);
            if (file.open(QFile::ReadOnly))
            {
                QTextStream stream(&file);
                while (!stream.atEnd())
                {
                    QString text = stream.readLine();
                    if (text.startsWith('*'))
                        continue;
                    
                    if (!text.contains('.'))
                        text = text.replace(',', '.');
                    QRegularExpressionMatchIterator it = numberRegEx.globalMatch(text);
                    while (it.hasNext())
                    {
                        QRegularExpressionMatch match = it.next();
                        QString freqString = match.captured();
                        bool ok;
                        double freq = freqString.toDouble(&ok);
                        if (ok && it.hasNext())
                        {
                            match = it.next();
                            QString gainString = match.captured();
                            double gain = gainString.toDouble(&ok);
                            if (ok)
                                newNodes.push_back(FilterNode(freq, gain));
                        }
                    }
                }
            }
        }
        sort(newNodes.begin(), newNodes.end());
        
        scene->setNodes(newNodes);
        
        int bandCount = scene->verifyBands(newNodes);
        if (bandCount != scene->getBandCount())
        {
            switch (bandCount)
            {
            case 15:
                ui->radioButton15->click();
                break;
            case 31:
                ui->radioButton31->click();
                break;
            default:
                ui->radioButtonVar->click();
            }
        }
        else
        {
            emit updateModel(false);
        }
    }
}

void GraphicEQFilterGUI::on_actionImport_AutoEQ_result_triggered()
{
    QFileInfo fileInfo(configPath);
    QFileDialog dialog(this, tr("Import EqualizerAPO graphic EQ preset"), fileInfo.absolutePath(), "*.txt");
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList nameFilters;
    nameFilters.append(tr("EqualizerAPO GraphicEQ (*.txt)"));
    nameFilters.append(tr("All files (*.*)"));
    dialog.setNameFilters(nameFilters);
    if (dialog.exec() == QDialog::Accepted)
    {
        QFile file(dialog.selectedFiles().at(0));
        if (file.open(QFile::ReadOnly))
        {
            QTextStream stream(&file);
            load(file.readAll());
        }
    }
}

void GraphicEQFilterGUI::on_actionExport_triggered()
{
    QFileInfo fileInfo(configPath);
    QFileDialog dialog(this, tr("Export frequency response"), fileInfo.absolutePath(), "*.csv");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QStringList nameFilters;
    nameFilters.append(tr("CSV frequency response (*.csv)"));
    nameFilters.append(tr("All files (*.*)"));
    dialog.setNameFilters(nameFilters);
    dialog.setDefaultSuffix(".csv");
    
    if (dialog.exec() == QDialog::Accepted)
    {
        QString savePath = dialog.selectedFiles().first();
        
        QFile file(savePath);
        if (file.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream stream(&file);
            for (FilterNode node : scene->getNodes())
            {
                stream << node.freq << '\t' << node.dbGain << '\n';
            }
            stream.flush();
        }
    }
}

void GraphicEQFilterGUI::on_actionInvertResponse_triggered()
{
    vector<FilterNode> newNodes = scene->getNodes();
    for (FilterNode& node : newNodes)
        node.dbGain = -node.dbGain;
    
    scene->setNodes(newNodes);
    emit updateModel(false);
}

void GraphicEQFilterGUI::on_actionNormalizeResponse_triggered()
{
    vector<FilterNode> newNodes = scene->getNodes();
    
    double maxGain = -DBL_MAX;
    for (FilterNode node : newNodes)
    {
        if (node.dbGain > maxGain)
            maxGain = node.dbGain;
    }
    
    if (maxGain != 0)
    {
        for (FilterNode& node : newNodes)
            node.dbGain -= maxGain;
        
        scene->setNodes(newNodes);
        emit updateModel(false);
    }
}

void GraphicEQFilterGUI::on_actionResetResponse_triggered()
{
    vector<FilterNode> newNodes = scene->getNodes();
    QSet<int> selectedIndices = scene->getSelectedIndices();
    
    int index = 0;
    for (FilterNode& node : newNodes)
    {
        if (selectedIndices.isEmpty() || selectedIndices.contains(index))
            node.dbGain = 0.0;
        index++;
    }
    
    scene->setNodes(newNodes);
    scene->setSelectedNodes(selectedIndices);
    emit updateModel(false);
}
