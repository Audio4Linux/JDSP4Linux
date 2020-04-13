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

#ifndef WEBLOADER_H
#define WEBLOADER_H

#include "NetworkRequest.h"
#include "customcookiejar.h"

#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

class QNetworkAccessManager;
class CustomCookieJar;
class WebRequest;

/*!
  \class WebLoader

  \brief Класс для работы с http-протоколом
  */
class WebLoader : public QThread
{
	Q_OBJECT

public:

public:
    explicit WebLoader(QObject* _parent = 0, CustomCookieJar* _jar = 0);
	virtual ~WebLoader();

	/*!
      \brief Установка куков для сессии загрузчика
	  */
    void setCookieJar(CustomCookieJar* _cookieJar);

    /*!
      \brief Установка метода запроса
	  */
    void setRequestMethod(NetworkRequest::RequestMethod _method);

    /**
     * @brief Установить таймаут загрузки
     */
    void setLoadingTimeout(int _msecs);

    /*!
     * \brief Установка WebRequest
     */
    void setWebRequest(WebRequest* _request);

	/*!
      \brief Отправка запроса (асинхронное выполнение)
      */
    void loadAsync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());

    /**
     * @brief Остановить выполнение
     */
    void stop();

signals:
	/*!
      \brief Прогресс отправки запроса на сервер
	  */
    void uploadProgress(int, QUrl);

    /*!
      \brief Прогресс загрузки данных с сервера
	  */
    void downloadProgress(int, QUrl);

    /*!
      \brief Данные загружены
      */
    void downloadComplete(QByteArray, QUrl);

    /*!
      \brief Сигнал об ошибке
	  */
    void error(QString, QUrl);
    void errorDetails(QString, QUrl);


//*****************************************************************************
// Внутренняя реализация класса

private:
	/*!
      \brief Работа потока
	  */
	void run();


private slots:
	/*!
      \brief Прогресс отправки запроса на сервер
      * uploadedBytes - отправлено байт, totalBytes - байт к отправке
	  */
    void uploadProgress(qint64 _uploadedBytes, qint64 _totalBytes);

    /*!
      \brief Прогресс загрузки данных с сервера
      * recievedBytes загружено байт, totalBytes - байт к отправке
	  */
    void downloadProgress(qint64 _recievedBytes, qint64 _totalBytes);

    /*!
      \brief Окончание загрузки страницы
	  */
    void downloadComplete(QNetworkReply* _reply);

    /*!
      \brief Ошибка при загрузки страницы
	  */
    void downloadError(QNetworkReply::NetworkError _networkError);

    /*!
     * \brief Ошибки при защищённом подключении
	 */
	void downloadSslErrors(const QList<QSslError>& _errors);


//*****************************************************************************
// Методы доступа к данным класса, а так же вспомогательные
// методы для работы с данными класса
private:
	void initNetworkManager();

// Данные класса
private:
    QNetworkAccessManager* m_networkManager;
    CustomCookieJar* m_cookieJar;
    WebRequest* m_request;
    NetworkRequest::RequestMethod m_requestMethod;
	bool m_isNeedRedirect;
    QUrl m_initUrl;

    /**
     * @brief Таймаут загрузки ссылки
     */
    int m_loadingTimeout;

	QByteArray m_downloadedData;
	QString m_lastError;
	QString m_lastErrorDetails;
};

#endif // WEBLOADER_H
