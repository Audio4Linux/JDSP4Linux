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

#ifndef FLATTABWIDGET_H
#define FLATTABWIDGET_H

#include <QWidget>
#include <QMap>
#include <QStackedWidget>
#include <QApplication>
#include <QPropertyAnimation>
#include <QBoxLayout>
#include "clickablelabel.h"

#include <3rdparty/StackedWidgetAnimation/StackedWidgetAnimation.h>

QT_BEGIN_NAMESPACE
namespace Ui { class FlatTabWidget; }
QT_END_NAMESPACE

class FlatTabItem;
/*!
    \class FlatTabWidget

    \brief The FlatTabWidget class provides a custom tab widget written from scratch with advanced animation capabilities.
*/
class FlatTabWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool animatePageChange READ getAnimatePageChange WRITE setAnimatePageChange)
    Q_PROPERTY(bool detachCustomStackedWidget READ getDetachCustomStackedWidget WRITE setDetachCustomStackedWidget)
public:
    FlatTabWidget(QWidget *parent = nullptr);
    ~FlatTabWidget();
    /*!
        \brief The TabPosition enum contains tab positions for use with \c setTabPosition and \c tabPosition
    */
    enum class TabPosition{
        Top,
        Bottom
    };
    /*!
        \brief Add a new page.\n
               By default, the page will be appended to the page list, \n
               unless \c index is set and greater or equal than 0.
    */
    void addPage(QString title, QWidget *page = nullptr, int index = -1);
    /*!
        \brief Remove page by index
    */
    void removePage(int index);
    /*!
        \brief Set the current selected tab. UI changes will be animated.
    */
    void setCurrentTab(int index);
    /*!
        \brief Get index of current selected tab
    */
    int getCurrentTab();
    /*!
        \brief Repaint divider\n
               Call this function if the separator appears displaced
    */
    void repaintDivider();
    /*!
        \brief Get tab id by title
    */
    int getId(QString title) const;
    /*!
        \brief Get tab item by id
    */
    FlatTabItem getItem(int id) const;
    /*!
        \brief Get tab item by title
    */
    FlatTabItem getItem(QString title) const;
    /*!
        \brief Check if page changes are animated (fade in/out)
    */
    bool getAnimatePageChange() const;
    /*!
        \brief Enable page change animations (fade in/out)
    */
    void setAnimatePageChange(bool value);
    /*!
        \brief Get custom stacked widget\n
               If none is set it will return a null pointer
    */
    QStackedWidget* getCustomStackWidget();
    /*!
        \brief Set a custom stacked widget\n
               Submit a null pointer to reset to the default stacked widget
    */
    void setCustomStackWidget(QStackedWidget* value);
    /*!
        \brief Get custom stacked widget detachment state
    */
    bool getDetachCustomStackedWidget() const;
    /*!
        \brief Set custom stacked widget detachment state\n
               If this is set to \c true, the tab widget will no longer add/remove pages to/from the stacked widget.\n
               This is useful if you want to use a custom stacked widget with already inserted/manually handled page contents.
    */
    void setDetachCustomStackedWidget(bool value);
    /*!
        \brief Redraw the tabbar\n
               Neccesary when updating the global palette
    */
    void redrawTabBar();
private slots:
    void lblHandler();

signals:
    /*!
        \brief Signal is sent when the user presses one of the tab buttons.\n
               Its first argument \c int \c index describes the current index.
    */
    void tabClicked(int index);
    /*!
        \brief Signal is sent when the user scrolls on the tabbar up
    */
    void scrolledUp();
    /*!
        \brief Signal is sent when the user scrolls on the tabbar down
    */
    void scrolledDown();
private:
    enum class ColorRole{
        Active,
        Inactive
    };
    Ui::FlatTabWidget *ui;
    int currentSelection;
    QPropertyAnimation *lineMorph;
    QVector<FlatTabItem> pages;
    bool enqueueSeparatorRepaint = true;
    bool animatePageChange = true;
    QStackedWidget* customSW = nullptr;
    bool detachCustomStackedWidget = false;
    TabPosition tabPosition = TabPosition::Top;
    WAF::AbstractAnimator* fade = nullptr;

    void updatePages(bool overrideSeparator = false);
    QStackedWidget *getActiveStackedWidget();
    QColor getColor(ColorRole role) const;

    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

    enum InsertPosition { InsertBefore, InsertAfter };
    static bool insertWidget(QBoxLayout * layout, QWidget * reference, QWidget * widget,
                      InsertPosition pos = InsertBefore, int stretch = 0,
                      Qt::Alignment alignment = 0) {
        int index = -1;
        for (int i = 0; i < layout->count(); ++i)
            if (layout->itemAt(i)->widget() == reference) {
                index = i;
                break;
            }
        if (index < 0) return false;
        if (pos == InsertAfter) index++;
        layout->insertWidget(index, widget, stretch, alignment);
        return true;
    }
};

/*!
    \class FlatTabItem

    \brief The FlatTabItem class provides a simple item model for the custom tab widget class
*/
class FlatTabItem{
public:
    FlatTabItem(ClickableLabel* _label = nullptr, QWidget* _widget = nullptr){
        label = _label;
        widget = _widget;
        fadeIn = new QPropertyAnimation();
        fadeOut = new QPropertyAnimation();
    }
    /*!
        \brief Check validity of an item
    */
    bool isValid(){
        if(label == nullptr || widget == nullptr)
            return false;
        return true;
    }
    ClickableLabel* label;
    QWidget* widget;
    QPropertyAnimation* fadeIn;
    QPropertyAnimation* fadeOut;
};
#endif // FLATTABWIDGET_H

