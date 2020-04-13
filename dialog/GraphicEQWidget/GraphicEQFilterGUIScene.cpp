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

#include <algorithm>
#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

#include "widgets/FrequencyPlotView.h"
#include "GraphicEQFilterGUIScene.h"

using namespace std;

GraphicEQFilterGUIScene::GraphicEQFilterGUIScene(QObject* parent)
	: FrequencyPlotScene(parent)
{
}

void GraphicEQFilterGUIScene::setNodes(const std::vector<FilterNode>& nodes)
{
	noUpdateModel = true;

	for (int i = (int)this->nodes.size() - 1; i >= 0; i--)
		removeNode(i);

	this->nodes = nodes;

	int i = 0;
	for (FilterNode node : nodes)
	{
		GraphicEQFilterGUIItem* item = new GraphicEQFilterGUIItem(i, node.freq, node.dbGain);
		addItem(item);
		item->updatePos();
		items.append(item);
		emit nodeInserted(i, node.freq, node.dbGain);
		i++;
	}

	update();

	noUpdateModel = false;
}

void GraphicEQFilterGUIScene::addNode(double hz, double db)
{
	FilterNode node(hz, db);
	vector<FilterNode>::iterator it = lower_bound(nodes.begin(), nodes.end(), node);
	int index = it - nodes.begin();
	nodes.insert(it, node);

	GraphicEQFilterGUIItem* item = new GraphicEQFilterGUIItem(index, hz, db);
	addItem(item);
	item->updatePos();
	items.insert(index, item);
	for (int i = index + 1; i < items.size(); i++)
		items[i]->setIndex(i);
	emit nodeInserted(index, hz, db);

	QRectF rect = sceneRect();
	if (index > 0)
		rect.setLeft(items[index - 1]->x());
	if (index + 1 < items.size())
		rect.setRight(items[index + 1]->x());
	update(rect);
	for (int i = index + 1; i < items.size(); i++)
		items[i]->update();

	if (!noUpdateModel)
		emit updateModel();
}

void GraphicEQFilterGUIScene::removeNode(int index)
{
	nodes.erase(nodes.begin() + index);
	GraphicEQFilterGUIItem* item = items[index];
	removeItem(item);
	items.removeAt(index);
	for (int i = index; i < items.size(); i++)
		items[i]->setIndex(i);
	emit nodeRemoved(index);

	QRectF rect = sceneRect();
	if (index > 0)
		rect.setLeft(items[index - 1]->x());
	if (index < items.size())
		rect.setRight(items[index]->x());
	update(rect);
	for (int i = index; i < items.size(); i++)
		items[i]->update();

	if (!noUpdateModel)
		emit updateModel();
}

void GraphicEQFilterGUIScene::setNode(int index, double hz, double db)
{
	GraphicEQFilterGUIItem* item = items[index];
	item->setHz(hz);
	item->setDb(db);
	item->updatePos();

	FilterNode node(hz, db);
	vector<FilterNode>::iterator it = lower_bound(nodes.begin(), nodes.end(), node);
	int newIndex = it - nodes.begin();
	if (newIndex > index)
		newIndex--;
	if (newIndex != index)
	{
		items.move(index, newIndex);
		if (newIndex < index)
		{
			for (int i = index; i > newIndex; i--)
				nodes[i] = nodes[i - 1];
			for (int i = newIndex; i <= index; i++)
				items[i]->setIndex(i);
			emit nodeMoved(index, newIndex);
		}
		else
		{
			for (int i = index; i < newIndex; i++)
				nodes[i] = nodes[i + 1];
			for (int i = index; i <= newIndex; i++)
				items[i]->setIndex(i);
			emit nodeMoved(index, newIndex);
		}
	}
	nodes[newIndex] = node;
	emit nodeUpdated(newIndex, hz, db);

	QRectF rect = sceneRect();
	int lowerIndex = min(index, newIndex);
	int upperIndex = max(index, newIndex);
	if (lowerIndex > 0)
		rect.setLeft(items[lowerIndex - 1]->x());
	if (upperIndex + 1 < items.size())
		rect.setRight(items[upperIndex + 1]->x());
	update(rect);

	if (!noUpdateModel)
		emit updateModel();
}

void GraphicEQFilterGUIScene::setSelectedNodes(QSet<int> indices)
{
	for (GraphicEQFilterGUIItem* item : items)
		item->setSelected(indices.contains(item->getIndex()));
}

QSet<int> GraphicEQFilterGUIScene::getSelectedIndices()
{
	QSet<int> set;

	for (GraphicEQFilterGUIItem* item : items)
		if (item->isSelected())
			set.insert(item->getIndex());

	return set;
}

void GraphicEQFilterGUIScene::itemMoved(int index)
{
	GraphicEQFilterGUIItem* item = items[index];
	double db = yToDb(item->scenePos().y());
	double oldHz = item->getHz();
	double hz;
	if (getBandCount() != -1)
	{
		hz = oldHz;
	}
	else
	{
		hz = xToHz(item->scenePos().x());
		if (hz >= 10000)
			hz = round(hz);
		else
			hz = round(hz * 10) / 10;
		item->setHz(hz);
	}
	if (getZoomY() < 4)
		db = round(db * 10) / 10;
	else
		db = round(db * 100) / 100;
	item->setDb(db);

	int newIndex = index;
	if (hz < oldHz)
	{
		while (newIndex > 0 && nodes[newIndex - 1].freq > hz)
		{
			nodes[newIndex] = nodes[newIndex - 1];
			items[newIndex] = items[newIndex - 1];
			items[newIndex]->setIndex(newIndex);
			newIndex--;
		}
	}
	else if (hz > oldHz)
	{
		while (newIndex < (int)nodes.size() - 1 && nodes[newIndex + 1].freq < hz)
		{
			nodes[newIndex] = nodes[newIndex + 1];
			items[newIndex] = items[newIndex + 1];
			items[newIndex]->setIndex(newIndex);
			newIndex++;
		}
	}
	nodes[newIndex] = FilterNode(hz, db);
	if (newIndex != index)
	{
		items[newIndex] = item;
		item->setIndex(newIndex);
		emit nodeMoved(index, newIndex);
	}
	emit nodeUpdated(newIndex, hz, db);

	QRectF rect = sceneRect();
	int lowerIndex = min(index, newIndex);
	int upperIndex = max(index, newIndex);
	if (lowerIndex > 0)
		rect.setLeft(items[lowerIndex - 1]->x());
	if (upperIndex + 1 < items.size())
		rect.setRight(items[upperIndex + 1]->x());
	update(rect);

	if (!noUpdateModel)
		emit updateModel();
}

void GraphicEQFilterGUIScene::itemSelectionChanged(int index, bool selected)
{
	emit nodeSelectionChanged(index, selected);
}

void GraphicEQFilterGUIScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	if (getBandCount() != -1)
		return;

	double hz = xToHz(event->scenePos().x());
	double db = yToDb(event->scenePos().y());
	addNode(hz, db);
}

void GraphicEQFilterGUIScene::keyPressEvent(QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Delete:
		if (getBandCount() == -1)
		{
			for (QGraphicsItem* item : selectedItems())
			{
				GraphicEQFilterGUIItem* plotItem = qgraphicsitem_cast<GraphicEQFilterGUIItem*>(item);
				if (plotItem != NULL)
				{
					// this works for multiple items because the index of the other items is updated inside removeNode
					int index = plotItem->getIndex();
					removeNode(index);
				}
			}
		}
		break;

	case Qt::Key_Up:
	case Qt::Key_Down:
	case Qt::Key_Left:
	case Qt::Key_Right:
		for (QGraphicsItem* item : selectedItems())
		{
			GraphicEQFilterGUIItem* plotItem = qgraphicsitem_cast<GraphicEQFilterGUIItem*>(item);
			if (plotItem != NULL)
			{
				int index = plotItem->getIndex();
				FilterNode node = nodes[index];
				switch (event->key())
				{
				case Qt::Key_Up:
					node.dbGain += 1;
					break;
				case Qt::Key_Down:
					node.dbGain -= 1;
					break;
				case Qt::Key_Left:
					node.freq -= 1;
					break;
				case Qt::Key_Right:
					node.freq += 1;
					break;
				}
				setNode(index, node.freq, node.dbGain);
			}
		}
		break;

	case Qt::Key_A:
		if (event->modifiers() & Qt::ControlModifier)
		{
			QPainterPath painterPath;
			painterPath.addRect(sceneRect());
			setSelectionArea(painterPath);
		}
		break;
	}
}

vector<FilterNode>& GraphicEQFilterGUIScene::getNodes()
{
	return nodes;
}

int GraphicEQFilterGUIScene::verifyBands(const std::vector<FilterNode>& nodes)
{
	const vector<double>& bands = getBands((int)nodes.size());

	int bandCount = -1;
	if (!bands.empty())
	{
		bandCount = (int)nodes.size();
		for (unsigned i = 0; i < nodes.size(); i++)
		{
			if (abs(nodes[i].freq - bands[i]) > 0.1)
			{
				bandCount = -1;
				break;
			}
		}
	}

	return bandCount;
}

void GraphicEQFilterGUIScene::setBandCount(int value)
{
	if (value != getBandCount())
	{
		const vector<double>& bands = getBands(value);
		if (!bands.empty())
		{
			vector<FilterNode> newNodes;
			GainIterator gainIterator(nodes);
			for (double band : bands)
			{
				double dbGain = gainIterator.gainAt(band);
				dbGain = round(dbGain * 100) / 100;
				FilterNode node(band, dbGain);
				newNodes.push_back(node);
			}

			setNodes(newNodes);
		}

		FrequencyPlotScene::setBandCount(value);

		if (!noUpdateModel)
			emit updateModel();
	}
}
