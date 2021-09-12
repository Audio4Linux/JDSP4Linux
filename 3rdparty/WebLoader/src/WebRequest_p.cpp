/*
* Copyright (C) 2015 Dimka Novikov, to@dimkanovikov.pro
* Copyright (C) 2016 Alexey Polushkin, armijo38@yandex.ru
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* Full license: http://dimkanovikov.pro/license/LGPLv3
*/

#include "WebRequest_p.h"
#include "HttpMultiPart_p.h"


#include <QFile>
#include <QStringList>
#include <QSslConfiguration>
#include <QMimeDatabase>

//! Заголовки запроса
const QByteArray USER_AGENT_HEADER = "User-Agent";
const QByteArray REFERER_HEADER = "Referer";
//! Параметры запроса
// UserAgent запроса
const QByteArray USER_AGENT = "Web Robot v.0.12.1";
// Boundary для разделения атрибутов запроса, при использовании  multi-part form data
const QString BOUNDARY = "---------------------------7d935033608e2";
// ContentType запроса
const QString CONTENT_TYPE_DEFAULT = "application/x-www-form-urlencoded";
const QString CONTENT_TYPE = "multipart/form-data; boundary=" + BOUNDARY;


WebRequest::WebRequest()
{

}

WebRequest::~WebRequest()
{

}

QUrl WebRequest::urlToLoad() const
{
	return m_urlToLoad;
}

void WebRequest::setUrlToLoad(const QUrl& _url)
{
    if (urlToLoad() != _url)
        m_urlToLoad = _url;
}

QUrl WebRequest::urlReferer() const
{
	return m_urlReferer;
}

void WebRequest::setUrlReferer(const QUrl& _url)
{
    if (urlReferer() != _url)
        m_urlReferer = _url;
}

void WebRequest::clearAttributes()
{
	m_attributes.clear();
	m_attributeFiles.clear();
    m_rawData.clear();
}

void WebRequest::addAttribute(const QString& _name, const QVariant& _value)
{
    if(m_usedRaw && !m_rawData.isEmpty()) {
        qWarning() << "You are trying to mix methods. Raw data will be cleaned";
        m_rawData.clear();
    }
    m_usedRaw = false;

	QPair< QString, QVariant > attribute;
    attribute.first = _name;
    attribute.second = _value;
    addAttribute(attribute);
}

void WebRequest::addAttributeFile(const QString& _name, const QString& _filePath)
{
    if(m_usedRaw && !m_rawData.isEmpty()) {
        qWarning() << "You are trying to mix methods. Raw data will be cleaned";
        m_rawData.clear();
    }
    m_usedRaw = false;

	QPair< QString, QString > attributeFile;
    attributeFile.first = _name;
    attributeFile.second = _filePath;
    addAttributeFile(attributeFile);
}

void WebRequest::setRawRequest(const QByteArray &_data)
{
    setRawRequest(_data, QMimeDatabase().mimeTypeForData(_data).name());
}

void WebRequest::setRawRequest(const QByteArray &_data, const QString &_mime)
{
    if(!m_usedRaw && (!m_attributes.isEmpty() || !m_attributeFiles.isEmpty())) {
        qWarning() << "You are trying to mix methods. Attributes will be cleaned";
        m_attributes.clear();
        m_attributeFiles.clear();
    }
    m_usedRaw = true;
    m_rawData = _data;
    m_mimeRawData = _mime;
}

QNetworkRequest WebRequest::networkRequest(bool _addContentHeaders)
{
    QNetworkRequest request(urlToLoad());
	// Установка заголовков запроса
	// User-Agent
    request.setRawHeader(USER_AGENT_HEADER, USER_AGENT);
	// Referer
    if (!urlReferer().isEmpty())
        request.setRawHeader(REFERER_HEADER, urlReferer().toString().toUtf8().data());
	// ContentType по-умолчанию
    request.setHeader(QNetworkRequest::ContentTypeHeader, CONTENT_TYPE_DEFAULT);

    if (_addContentHeaders) {
        // ContentType
        if(m_usedRaw) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, m_mimeRawData);
        }
        else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, CONTENT_TYPE);
        }
		// ContentLength
        request.setHeader(QNetworkRequest::ContentLengthHeader, multiPartData().size());
	}

	return request;
}

QByteArray WebRequest::multiPartData()
{
    if(m_usedRaw) {
        return m_rawData;
    }

	HttpMultiPart multiPart;
    multiPart.setBoundary(BOUNDARY);

	// Добавление текстовых атрибутов
	QPair< QString, QVariant > attribute;
    foreach (attribute, attributes()) {
		QString attributeName  = attribute.first;
		QString attributeValue = attribute.second.toString();

        HttpPart textPart(HttpPart::Text);
        textPart.setText(attributeName, attributeValue);

        multiPart.addPart(textPart);
	}

	// Добавление атрибутов-файлов
	QPair< QString, QString > attributeFile;
    foreach (attributeFile, attributeFiles()) {
		QString attributeName     = attributeFile.first;
		QString attributeFilePath = attributeFile.second;

        HttpPart filePart(HttpPart::File);
        filePart.setFile(attributeName, attributeFilePath);

        multiPart.addPart(filePart);
	}

	return multiPart.data();
}


//*****************************************************************************
// Методы доступа к данным класса, а так же вспомогательные
// методы для работы с данными класса

QList<QPair<QString, QVariant> > WebRequest::attributes() const
{
	return m_attributes;
}

void WebRequest::addAttribute(const QPair<QString, QVariant>& _attribute)
{
    if (!attributes().contains(_attribute))
        m_attributes.append(_attribute);
}

QList<QPair<QString, QString> > WebRequest::attributeFiles() const
{
	return m_attributeFiles;
}

void WebRequest::addAttributeFile(const QPair<QString, QString>& _attributeFile)
{
    if (!attributeFiles().contains(_attributeFile))
        m_attributeFiles.append(_attributeFile);
}
