/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
*/

#include "flattabwidget.h"
#include "ui_flattabwidget.h"
#include "colorprovider.h"

#include <QEasingCurve>

using namespace ColorProvider;

FlatTabWidget::FlatTabWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FlatTabWidget)
{
    ui->setupUi(this);

    lineMorph = new QPropertyAnimation();

    connect(ui->TabBarContainer,&CustomTabBar::scrolledUp,[=]{
        if(currentSelection - 1 < 0 || pages.empty())
            return;

        emit scrolledUp();
        auto item = pages[currentSelection - 1];
        if(item.label != nullptr){
            setCurrentTab(currentSelection - 1);
        }
    });
    connect(ui->TabBarContainer,&CustomTabBar::scrolledDown,[=]{
        if(currentSelection + 1 >= pages.size() || pages.empty())
            return;

        emit scrolledDown();
        auto item = pages[currentSelection + 1];
        if(item.label != nullptr){
            setCurrentTab(currentSelection + 1);
        }
    });
}

FlatTabWidget::~FlatTabWidget()
{
    delete ui;
}

void FlatTabWidget::addPage(QString title, QWidget* page, int index){
    auto pal = palette();
    auto textcolor_active = getColor(ColorRole::Active);
    auto textcolor_disabled = getColor(ColorRole::Inactive);

    ClickableLabel* lbl = new ClickableLabel();
    lbl->setText(title);
    if(pages.empty()){
        lbl->setColor(textcolor_active);
        currentSelection = 0;
    } else {
        lbl->setColor(textcolor_disabled);
    }
    connect(lbl,&ClickableLabel::clicked,this,&FlatTabWidget::lblHandler);

    if(index < 0)
        pages.append(FlatTabItem(lbl,page));
    else
        pages.insert(index,FlatTabItem(lbl,page));
    updatePages();
}
void FlatTabWidget::updatePages(bool overrideSeparator){
    QStackedWidget* activeContainer = getActiveStackedWidget();
    auto spacer = ui->TabBarContainer->layout()->takeAt(ui->TabBarContainer->layout()->count()-1);
    while (auto wItem = ui->TabBarContainer->layout()->takeAt(0))
        if(wItem != 0){
            wItem->widget()->hide();
            delete wItem;
        }
    if(!detachCustomStackedWidget){
        while(activeContainer->count() > 0){
            QWidget* widget = activeContainer->widget(0);
            activeContainer->removeWidget(widget);
        }
    }
    ui->TabBarContainer->repaint();

    for (auto page : pages) {
        ui->TabBarContainer->layout()->addWidget(page.label);
        if(!detachCustomStackedWidget)
            activeContainer->addWidget(page.widget);
        page.label->show();
    }
    ui->TabBarContainer->layout()->addItem(spacer);

    if(overrideSeparator)
        repaintDivider();

    setCurrentTab(currentSelection);
}

void FlatTabWidget::paintEvent(QPaintEvent *event)
{
    if(enqueueSeparatorRepaint){
        if(pages.empty() || currentSelection < 0 || currentSelection >= pages.size())
            return;

        auto button = pages[currentSelection].label;
        auto rect = QRect(button->x(),0,button->width(),button->height());
        lineMorph->stop();
        ui->SeparatorLine->setGeometry(rect);
        enqueueSeparatorRepaint = false;
    }
    if(event->type() == QPaintEvent::PaletteChange)
        redrawTabBar();

    QWidget::paintEvent(event);
}

void FlatTabWidget::showEvent(QShowEvent *event)
{
    repaintDivider();
    setPalette(palette());
    QWidget::showEvent(event);
}

void FlatTabWidget::redrawTabBar(){
    for (auto page : pages)
        page.label->setColor(getColor(ColorRole::Inactive));
    int temp = currentSelection;
    setCurrentTab(temp);
}

void FlatTabWidget::removePage(int id){
    auto pal = palette();
    auto textcolor_disabled = getColor(ColorRole::Inactive);

    if(!pages.empty() && (id >= 0 && id < pages.size())){
        if(id < pages.size() - 1)
            pages[id + 1].label->setColor(textcolor_disabled);
        pages.remove(id);
    }
    updatePages();
}

void FlatTabWidget::setCurrentTab(int id){
    if(pages.empty())
        return;
    if(id < 0)
        id = 0;
    if(id >= pages.size())
        id = pages.size() - 1;

    QColor textcolor_active = getColor(ColorRole::Active);
    QColor textcolor_disabled = getColor(ColorRole::Inactive);
    ClickableLabel* button = pages[id].label;
    QRect rect = QRect(button->x(),0,button->width(),button->height());
    if(id == currentSelection){
        pages[id].label->setColor(textcolor_active);
        lineMorph->stop();
        ui->SeparatorLine->setGeometry(rect);
    } else {
        pages[id].fadeIn->stop();
        pages[id].fadeOut->stop();
        pages[id].fadeIn = new QPropertyAnimation(pages[id].label, "color");
        pages[id].fadeIn->setDuration(300);
        pages[id].fadeIn->setStartValue(textcolor_disabled);
        pages[id].fadeIn->setEndValue(textcolor_active);

        if(pages.size() > currentSelection){
            pages[currentSelection].fadeIn->stop();
            pages[currentSelection].fadeOut->stop();
            pages[currentSelection].fadeOut = new QPropertyAnimation(pages[currentSelection].label, "color");
            pages[currentSelection].fadeOut->setDuration(300);
            pages[currentSelection].fadeOut->setStartValue(textcolor_active);
            pages[currentSelection].fadeOut->setEndValue(textcolor_disabled);
            pages[currentSelection].fadeOut->start();
        }
        pages[id].fadeIn->start();
    }

    lineMorph->stop();
    lineMorph = new QPropertyAnimation(ui->SeparatorLine, "geometry");
    lineMorph->setDuration(300);
    lineMorph->setEasingCurve(QEasingCurve::OutCirc);
    lineMorph->setStartValue(ui->SeparatorLine->geometry());
    lineMorph->setEndValue(rect);
    lineMorph->start();

    if(animatePageChange){
        QWidget* widget = getActiveStackedWidget()->widget(id);
        if(fade != nullptr)
            fade->animateStop();
        if(widget != nullptr)
            fade = WAF::StackedWidgetAnimation::fadeIn(getActiveStackedWidget(), widget);

    }
    else
        getActiveStackedWidget()->setCurrentIndex(id);
    currentSelection = id;
}

int FlatTabWidget::getCurrentTab()
{
    return currentSelection;
}

void FlatTabWidget::repaintDivider()
{
    enqueueSeparatorRepaint = true;
    repaint();
}

QStackedWidget* FlatTabWidget::getActiveStackedWidget(){
    if(customSW != nullptr)
        return customSW;
    else
        return ui->Content;
}

QColor FlatTabWidget::getColor(FlatTabWidget::ColorRole role) const
{
    auto pal = palette();
    auto textcolor_active = pal.color(QPalette::ButtonText).darker(0);
    auto textcolor_disabled_light = pal.color(QPalette::Disabled,QPalette::WindowText).darker(150);
    auto textcolor_disabled_dark = pal.color(QPalette::Disabled,QPalette::WindowText).lighter(150);

    Hsl hsl0 = Hsl::ofQColor(pal.color(QPalette::WindowText));
    Hsl hsl1 = Hsl::ofQColor(pal.color(QPalette::Window));
    bool isLightPalette = hsl0.l < hsl1.l;

    if(role == ColorRole::Active)
        return textcolor_active;
    else
        return isLightPalette ? textcolor_disabled_light : textcolor_disabled_dark;
}

FlatTabItem FlatTabWidget::getItem(int id) const{
    if(id >= 0 && id < pages.size())
        return pages[id];
    return FlatTabItem();
}

FlatTabItem FlatTabWidget::getItem(QString title) const{
    int id = getId(title);
    if(id < 0)
        return FlatTabItem();
    else
        return getItem(id);
}

int FlatTabWidget::getId(QString title) const{
    for (int i = 0;i < pages.size();i++) {
        if(pages[i].label->text() == title)
            return i;
    }
    return -1;
}

void FlatTabWidget::lblHandler(){
    int id = getId(qobject_cast<ClickableLabel*>(sender())->text());
    if(id >= 0){
        setCurrentTab(id);
        emit tabClicked(id);
    }
}

bool FlatTabWidget::getDetachCustomStackedWidget() const
{
    return detachCustomStackedWidget;
}

void FlatTabWidget::setDetachCustomStackedWidget(bool value)
{
    detachCustomStackedWidget = value;
}

QStackedWidget* FlatTabWidget::getCustomStackWidget()
{
    return customSW;
}

void FlatTabWidget::setCustomStackWidget(QStackedWidget* value)
{
    customSW = value;
    if(value == nullptr)
        ui->Content->show();
    else
        ui->Content->hide();
}

bool FlatTabWidget::getAnimatePageChange() const
{
    return animatePageChange;
}
void FlatTabWidget::setAnimatePageChange(bool value)
{
    animatePageChange = value;
}
