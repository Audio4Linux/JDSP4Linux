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
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */
#ifndef STYLEHELPER_H
#define STYLEHELPER_H
#include <QObject>
#include <QColor>
#include "misc/initializableqmap.h"
#define CS_UNIT QString,ColorStyle

class ColorStyle;
class StyleHelper : public QObject
{
    Q_OBJECT
public:

    StyleHelper(QObject* host);
    void SetStyle();
    void loadIcons(bool white);
    int loadColor(int index,int rgb_index);
    void setPalette(const ColorStyle& s);
private:
    QObject* m_objhost;
signals:
    void styleChanged();
};

class ColorStyle {
public:
    bool useWhiteIcons;
    QColor background;
    QColor foreground;
    QColor base;
    QColor selection;
    QColor selectiontext;
    QColor disabled;

    ColorStyle(){}
    ColorStyle(const bool _whiteIcons, const QColor& _base,const QColor& _background,
               const QColor& _foreground,const QColor& _selection = QColor(42,130,218),
               const QColor& _selectiontext = Qt::black,const QColor& _disabled = QColor(85,85,85)){
        useWhiteIcons = _whiteIcons;
        base = _base;
        background = _background;
        foreground = _foreground;
        selection = _selection;
        selectiontext = _selectiontext;
        disabled = _disabled;
    }
};


namespace ColorStyleProvider {
static QMap<CS_UNIT> TABLE(){
    InitializableQMap<CS_UNIT> map;
    map << QPair<CS_UNIT>("dark",ColorStyle(true,QColor(25,25,25),QColor(53,53,53),Qt::white,QColor(42, 130, 218)))
        << QPair<CS_UNIT>("gray",ColorStyle(true,QColor(83,83,125),QColor(49,49,74),Qt::white,QColor(151, 151, 184),QColor(144,144,179)))
        << QPair<CS_UNIT>("blue",ColorStyle(true,QColor(0,0,38),QColor(0,0,50),Qt::white,QColor(85,0,255)))
        << QPair<CS_UNIT>("darkblue",ColorStyle(true,QColor(14,19,29),QColor(19,25,38),Qt::white,QColor(70,79,89)))
        << QPair<CS_UNIT>("honeycomb",ColorStyle(false,QColor(185,188,182),QColor(212,215,208),Qt::black,QColor(243,193,41),Qt::white))
        << QPair<CS_UNIT>("black",ColorStyle(true,Qt::black,QColor(28,28,28),QColor(222,222,222),QColor(132,132,132)))
        << QPair<CS_UNIT>("darkgreen",ColorStyle(true,QColor(30,30,30),QColor(27,34,36),QColor(197,209,217),QColor(153, 199, 190)))
        << QPair<CS_UNIT>("green",ColorStyle(true,QColor(6,29,12),QColor(0,12,0),Qt::white,QColor(86,191,121)))
        << QPair<CS_UNIT>("stone",ColorStyle(true,QColor(27,36,40),QColor(34,45,50),Qt::white,QColor(165,206,255)));
    return std::move(map);
}
}
#endif // STYLEHELPER_H
