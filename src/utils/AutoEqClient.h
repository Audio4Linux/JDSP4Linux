#ifndef AUTOEQCLIENT_H
#define AUTOEQCLIENT_H

#include "Log.h"

#include <QDebug>
#include <QObject>
#include <QPixmap>

#include <WebLoader/src/NetworkRequest.h>

#define ROOT_RAW_URL    "https://github.com/jaakkopasanen/AutoEq/raw/master"
#define RESULTS_RAW_URL ROOT_RAW_URL "/results"
#define INDEX_RAW_URL   RESULTS_RAW_URL "/INDEX.md"

#define ROOT_API_URL     "https://api.github.com/repos/jaakkopasanen/AutoEq"
#define CONTENTS_API_URL ROOT_API_URL "/contents"

class QueryResult;
class QueryRequest;
class HeadphoneMeasurement;

class AutoEQClient
{
public:
	static QVector<QueryResult> query(QueryRequest request);
	static HeadphoneMeasurement fetchDetails(QueryResult item);

};

class QueryRequest
{
public:
	QueryRequest(QString modelFilter = "",
	             QString groupFilter = "")
	{
		mModelFilter = modelFilter;
		mGroupFilter = groupFilter;
	}

	const QString getModelFilter()
	{
		return mModelFilter;
	}

	const QString getGroupFilter()
	{
		return mGroupFilter;
	}

	bool isModelFilterEnabled()
	{
		return !mModelFilter.isEmpty();
	}

	bool isGroupFilterEnabled()
	{
		return !mGroupFilter.isEmpty();
	}

	QString mModelFilter;
	QString mGroupFilter;
};

class QueryResult
{
public:
	QueryResult(QString model   = "",
	            QString group   = "",
	            QString apipath = "")
	{
		mModel   = model;
		mGroup   = group;
		mApiPath = apipath;
	}

	const QString getModel()
	{
		return mModel;
	}

	const QString getGroup()
	{
		return mGroup;
	}

	const QString getAPIPath()
	{
		return mApiPath;
	}

	QString mModel;
	QString mGroup;
	QString mApiPath;
};

class HeadphoneMeasurement
{
public:
	HeadphoneMeasurement(QString model     = "",
	                     QString group     = "",
	                     QString graphiceq = "",
	                     QString graphurl  = "")
	{
		mModel     = model;
		mGroup     = group;
		mGraphicEQ = graphiceq;
		mGraphUrl  = graphurl;
	}

	const QString getModel()
	{
		return mModel;
	}

	const QString getGroup()
	{
		return mGroup;
	}

	const QString getGraphicEQ()
	{
		return mGraphicEQ;
	}

	const QString getGraphUrl()
	{
		return mGraphUrl;
	}

	void setModel(QString model)
	{
		mModel = model;
	}

	void setGroup(QString group)
	{
		mGroup = group;
	}

	void setGraphicEQ(QString graphiceq)
	{
		mGraphicEQ = graphiceq;
	}

	void setGraphUrl(QString graphurl)
	{
		mGraphUrl = graphurl;
	}

	QPixmap getGraphImage()
	{
		NetworkRequest net_request;
		net_request.setRequestMethod(NetworkRequest::Get);

		QByteArray     image = net_request.loadSync(getGraphUrl());

		if (net_request.lastError() != "")
		{
			Log::error("An error occurred (getGraphImage): " + net_request.lastError());
			return QPixmap();
		}

		QPixmap pixmap;
		pixmap.loadFromData(image, "PNG");
		return pixmap;
	}

	QString mModel;
	QString mGroup;
	QString mGraphicEQ;
	QString mGraphUrl;
};

Q_DECLARE_METATYPE(QueryResult)

#endif // AUTOEQCLIENT_H