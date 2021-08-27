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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QWidget>
/*!
    \class ClickableLabel

    \brief The ClickableLabel class provides a simple label with a click signal and color property for color animation
*/
class ClickableLabel : public QLabel {
    Q_OBJECT
    Q_PROPERTY(QColor color WRITE setColor READ color)
public:
    explicit ClickableLabel(QWidget* parent = Q_NULLPTR);
    ~ClickableLabel();
    /*!
        \brief Set text color
    */
    void setColor(QColor color);
    /*!
        \brief Get text color\n
         \bold Warning: Usage not recommended
    */
    QColor color() const;

signals:
    /*!
        \brief User has clicked the label
    */
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event);

private:
    QColor mColor;
};

#endif // CLICKABLELABEL_H
