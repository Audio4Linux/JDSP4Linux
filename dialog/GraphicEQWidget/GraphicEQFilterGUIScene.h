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

#pragma once

#include <vector>

#include "widgets/FrequencyPlotScene.h"
#include "helpers/GainIterator.h"
#include "GraphicEQFilterGUIItem.h"

class GraphicEQFilterGUIScene : public FrequencyPlotScene
{
	Q_OBJECT
public:
	explicit GraphicEQFilterGUIScene(QObject* parent = 0);
	void setNodes(const std::vector<FilterNode>& nodes);
	void addNode(double hz, double db);
	void removeNode(int index);
	void setNode(int index, double hz, double db);
	void setSelectedNodes(QSet<int> indices);
	QSet<int> getSelectedIndices();
	void itemMoved(int index);
	void itemSelectionChanged(int index, bool selected);

	std::vector<FilterNode>& getNodes();
	int verifyBands(const std::vector<FilterNode>& nodes);
	void setBandCount(int value) override;

signals:
	void nodeInserted(int index, double hz, double db);
	void nodeRemoved(int index);
	void nodeUpdated(int index, double hz, double db);
	void nodeMoved(int fromIndex, int toIndex);
	void nodeSelectionChanged(int index, bool selected);
	void updateModel();

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	std::vector<FilterNode> nodes;
	QList<GraphicEQFilterGUIItem*> items;

	bool noUpdateModel = false;
};
