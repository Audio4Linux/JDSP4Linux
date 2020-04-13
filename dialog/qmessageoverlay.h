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
#ifndef QLIGHTBOXWIDGET_H
#define QLIGHTBOXWIDGET_H

#include <QWidget>

class QMessageOverlay : public QWidget
{
	Q_OBJECT

public:
    explicit QMessageOverlay(QWidget* _parent, bool _folowToHeadWidget = false);

protected:
	bool eventFilter(QObject* _object, QEvent* _event);
	void paintEvent(QPaintEvent* _event);
	void showEvent(QShowEvent* _event);

private:
	void updateSelf();
	bool m_isInUpdateSelf;
	QPixmap grabParentWidgetPixmap() const;
	QPixmap m_parentWidgetPixmap;
};

#endif // QLIGHTBOXWIDGET_H
