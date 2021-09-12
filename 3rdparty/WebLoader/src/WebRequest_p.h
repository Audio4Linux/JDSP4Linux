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

#ifndef WEBREQUEST_H
#define WEBREQUEST_H

#include <QString>
#include <QVariant>
#include <QNetworkRequest>


/*!
  \class WebLoader

  \brief Класс обертка для QNetworkRequest
  */
class WebRequest
{
public:
	WebRequest();
	~WebRequest();

	/*!
      \brief Ссылка запроса
	  */
	QUrl urlToLoad() const;

    /*!
      \brief Установка ссылки для запроса
	  */
    void setUrlToLoad(const QUrl& _url);

	/*!
      \brief Ссылка referer
	  */
	QUrl urlReferer() const;

    /*!
      \brief Установка ссылки referer'а
	  */
    void setUrlReferer(const QUrl& _url);

    /*!
     * \brief Очистить список атрибутов
	 */
	void clearAttributes();

    /*!
      \brief Добавление текстового атрибута в запрос
      * name - название атрибута, value - значение атрибута
	  */
    void addAttribute(const QString& _name, const QVariant& _value);

    /*!
      \brief Добавление атрибута-файла в запрос
      * name - название атрибута, filePath - путь к файлу
	  */
    void addAttributeFile(const QString& _name, const QString& _filePath);

    void setRawRequest(const QByteArray& _data);
    void setRawRequest(const QByteArray& _data, const QString& _mime);
	/*!
      \brief Сформированный объект класса QNetworkRequest
	  */
    QNetworkRequest networkRequest(bool _addContentHeaders = false);

	/*!
      \breif Атрибуты запроса
	  */
	QByteArray  multiPartData();

//*****************************************************************************
// Внутренняя реализация класса

private:
	/*!
      \brief Текстовые атрибуты запроса
	  */
	QList< QPair< QString, QVariant > > attributes() const;

    /*!
      \brief Добавление текстового атрибута в запрос
      * attribute - имя + значения атрибута
	  */
    void addAttribute(const QPair< QString, QVariant >& _attribute);

    /*!
      \brief Атрибуты-файлы запроса
	  */
	QList<QPair<QString, QString> > attributeFiles() const;

    /*!
      \brief Добавление атрибута-файла в запрос
      * attributeFile - имя атрибута + путь к файлу
	  */
    void addAttributeFile(const QPair<QString, QString>& _attributeFile);

private:
	QUrl m_urlToLoad,
		 m_urlReferer;
	QList< QPair< QString, QVariant > > m_attributes;
	QList< QPair< QString, QString > >  m_attributeFiles;

    QByteArray m_rawData;
    QString m_mimeRawData;
    bool m_usedRaw;
};

#endif // WEBREQUEST_H
