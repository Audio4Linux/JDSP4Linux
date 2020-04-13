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

#ifndef HTTPMULTIPART_H
#define HTTPMULTIPART_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QList>

class HttpPart
{
public:
	enum HttpPartType {
		Text,
		File
	};

public:
    HttpPart(HttpPartType _type = Text);
	HttpPartType type() const;
    void setText(const QString& _name, const QString& _value);
    void setFile(const QString& _name, const QString& _filePath);

public:
	QString name() const;
	QString value() const;
	QString fileName() const;
	QString filePath() const;


private:
    void setName(const QString& _name);
    void setValue(const QString& _value);
    void setFileName(const QString& _fileName);
    void setFilePath(const QString& _filePath);

private:
	HttpPartType m_type;
	QString m_name,
			m_value,
			m_filePath;
};

class HttpMultiPart
{
public:
	HttpMultiPart();
    void setBoundary(const QString& _boundary);
    void addPart(const HttpPart& _part);

	QByteArray data();

private:
    QByteArray makeDataFromPart(const HttpPart& _part);
    QByteArray makeDataFromTextPart(const HttpPart& _part);
    QByteArray makeDataFromFilePart(const HttpPart& _part);
	QByteArray makeEndData();

private:
	QString boundary() const;
	QString crlf() const;
	QList<HttpPart> parts() const;

private:
	QString m_boundary;
	QList<HttpPart> m_parts;
};

#endif // HTTPMULTIPART_H
