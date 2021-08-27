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

#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QObject>
#include <QWidget>
#include <QWheelEvent>
/*!
    \class CustomTabBar

    \brief The CustomTabBar class provides a tabbar overlay widget with scrolling capabilities
*/
class CustomTabBar : public QWidget
{
    Q_OBJECT
public:
    CustomTabBar(QWidget* parent = Q_NULLPTR);
    void wheelEvent(QWheelEvent *event) override;
signals:
    /*!
        \brief User has scrolled up
    */
    void scrolledUp();
    /*!
        \brief User has scrolled down
    */
    void scrolledDown();
};

#endif // CUSTOMTABBAR_H
