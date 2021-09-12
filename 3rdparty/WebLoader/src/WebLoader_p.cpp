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

#include "WebLoader_p.h"
#include "WebRequest_p.h"
#include "customcookiejar.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QHttpMultiPart>
#include <QtCore/QTimer>
#include <QEventLoop>
#include <QPointer>

namespace {
	/**
	 * @brief Не все сайты передают суммарный размер загружаемой страницы,
	 *		  поэтому для отображения прогресса загрузки используется
	 *		  заранее заданное число (средний размер веб-страницы)
	 */
	const int POSSIBLE_RECIEVED_MAX_FILE_SIZE = 120000;

	/**
	 * @brief Преобразовать ошибку в читаемый вид
	 */
	static QString networkErrorToString(QNetworkReply::NetworkError networkError) {
		QString result;
		switch (networkError) {
			case QNetworkReply::ConnectionRefusedError: result = "the remote server refused the connection (the server is not accepting requests)"; break;
			case QNetworkReply::RemoteHostClosedError: result = "the remote server closed the connection prematurely, before the entire reply was received and processed"; break;
			case QNetworkReply::HostNotFoundError: result = "the remote host name was not found (invalid hostname)"; break;
			case QNetworkReply::TimeoutError: result = "the connection to the remote server timed out"; break;
			case QNetworkReply::OperationCanceledError: result = "the operation was canceled via calls to abort() or close() before it was finished."; break;
			case QNetworkReply::SslHandshakeFailedError: result = "the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() signal should have been emitted."; break;
			case QNetworkReply::TemporaryNetworkFailureError: result = "the connection was broken due to disconnection from the network, however the system has initiated roaming to another access point. The request should be resubmitted and will be processed as soon as the connection is re-established."; break;
			case QNetworkReply::NetworkSessionFailedError: result = "the connection was broken due to disconnection from the network or failure to start the network."; break;
			case QNetworkReply::BackgroundRequestNotAllowedError: result = "the background request is not currently allowed due to platform policy."; break;
			case QNetworkReply::ProxyConnectionRefusedError: result = "the connection to the proxy server was refused (the proxy server is not accepting requests)"; break;
			case QNetworkReply::ProxyConnectionClosedError: result = "the proxy server closed the connection prematurely, before the entire reply was received and processed"; break;
			case QNetworkReply::ProxyNotFoundError: result = "the proxy host name was not found (invalid proxy hostname)"; break;
			case QNetworkReply::ProxyTimeoutError: result = "the connection to the proxy timed out or the proxy did not reply in time to the request sent"; break;
			case QNetworkReply::ProxyAuthenticationRequiredError: result = "the proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)"; break;
			case QNetworkReply::ContentAccessDenied: result = "the access to the remote content was denied (similar to HTTP error 401)"; break;
			case QNetworkReply::ContentOperationNotPermittedError: result = "the operation requested on the remote content is not permitted"; break;
			case QNetworkReply::ContentNotFoundError: result = "the remote content was not found at the server (similar to HTTP error 404)"; break;
			case QNetworkReply::AuthenticationRequiredError: result = "the remote server requires authentication to serve the content but the credentials provided were not accepted (if any)"; break;
			case QNetworkReply::ContentReSendError: result = "the request needed to be sent again, but this failed for example because the upload data could not be read a second time."; break;
			case QNetworkReply::ContentConflictError: result = "the request could not be completed due to a conflict with the current state of the resource."; break;
			case QNetworkReply::ContentGoneError: result = "the requested resource is no longer available at the server."; break;
			case QNetworkReply::InternalServerError: result = "the server encountered an unexpected condition which prevented it from fulfilling the request."; break;
			case QNetworkReply::OperationNotImplementedError: result = "the server does not support the functionality required to fulfill the request."; break;
			case QNetworkReply::ServiceUnavailableError: result = "the server is unable to handle the request at this time."; break;
			case QNetworkReply::ProtocolUnknownError: result = "the Network Access API cannot honor the request because the protocol is not known"; break;
			case QNetworkReply::ProtocolInvalidOperationError: result = "the requested operation is invalid for this protocol"; break;
			case QNetworkReply::UnknownNetworkError: result = "an unknown network-related error was detected"; break;
			case QNetworkReply::UnknownProxyError: result = "an unknown proxy-related error was detected"; break;
			case QNetworkReply::UnknownContentError: result = "an unknown error related to the remote content was detected"; break;
			case QNetworkReply::ProtocolFailure: result = "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)"; break;
			case QNetworkReply::UnknownServerError: result = "an unknown error related to the server response was detected"; break;
#if QT_VERSION >= 0x050600
			case QNetworkReply::TooManyRedirectsError: result = "while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set by QNetworkRequest::setMaxRedirectsAllowed()."; break;
			case QNetworkReply::InsecureRedirectError: result = "while following redirects, the network access API detected a redirect from a encrypted protocol (https) to an unencrypted one (http)."; break;
#endif
			case QNetworkReply::NoError: result = "No error"; break;
		}

		return result;
	}
}


WebLoader::WebLoader(QObject* _parent, CustomCookieJar* _jar) :
	QThread(_parent),
	m_networkManager(0),
	m_cookieJar(_jar),
	m_request(new WebRequest),
	m_requestMethod(NetworkRequest::Undefined),
	m_isNeedRedirect(true),
	m_loadingTimeout(20000)
{
}

WebLoader::~WebLoader()
{
	if (m_networkManager)
		m_networkManager->deleteLater();//delete m_networkManager;//
}

void WebLoader::setCookieJar(CustomCookieJar* _jar)
{
	if (m_cookieJar != _jar)
		m_cookieJar = _jar;
}

void WebLoader::setRequestMethod(NetworkRequest::RequestMethod _method)
{
	if (m_requestMethod != _method)
		m_requestMethod = _method;
}

void WebLoader::setLoadingTimeout(int _msecs)
{
	if (m_loadingTimeout != _msecs) {
		m_loadingTimeout = _msecs;
	}
}

void WebLoader::setWebRequest(WebRequest* _request) {
	this->m_request = _request;
}

void WebLoader::loadAsync(const QUrl& _urlToLoad, const QUrl& _referer)
{
	stop();

	m_request->setUrlToLoad(_urlToLoad);
	m_request->setUrlReferer  (_referer);

	start();
}


//*****************************************************************************
// Внутренняя реализация класса

void WebLoader::run()
{
	initNetworkManager();

	m_initUrl = m_request->urlToLoad();

	do
	{
		//! Начало загрузки страницы m_request->url()
		emit uploadProgress(0, m_initUrl);
		emit downloadProgress(0, m_initUrl);

		QPointer<QNetworkReply> reply = 0;

		switch (m_requestMethod) {

			default:
			case NetworkRequest::Get: {
				const QNetworkRequest request = this->m_request->networkRequest();
				reply = m_networkManager->get(request);
				break;
			}

			case NetworkRequest::Post: {
				const QNetworkRequest networkRequest = m_request->networkRequest(true);
				const QByteArray data = m_request->multiPartData();
				reply = m_networkManager->post(networkRequest, data);
				break;
			}

		} // switch

		connect(reply.data(), &QNetworkReply::uploadProgress,
				this, static_cast<void (WebLoader::*)(qint64, qint64)>(&WebLoader::uploadProgress));
		connect(reply.data(), &QNetworkReply::downloadProgress,
				this, static_cast<void (WebLoader::*)(qint64, qint64)>(&WebLoader::downloadProgress));
		connect(reply.data(), static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
				this, &WebLoader::downloadError);
		connect(reply.data(), &QNetworkReply::sslErrors, this, &WebLoader::downloadSslErrors);
		connect(reply.data(), &QNetworkReply::sslErrors,
				reply.data(), static_cast<void (QNetworkReply::*)()>(&QNetworkReply::ignoreSslErrors));

		//
		// Таймер для прерывания работы
		//
		QTimer timeoutTimer;
		connect(&timeoutTimer, &QTimer::timeout, this, &WebLoader::quit);
		connect(&timeoutTimer, &QTimer::timeout, reply.data(), &QNetworkReply::abort);
		timeoutTimer.setSingleShot(true);
		timeoutTimer.start(m_loadingTimeout);

		//
		// Входим в поток обработки событий, ожидая завершения отработки networkManager'а
		//
		exec();

		//
		// Если ответ ещё не удалён
		//
		if (!reply.isNull()) {
			//
			// ... если ответ получен, останавливаем таймер
			//
			if (reply->isFinished()) {
				timeoutTimer.stop();
			}
			//
			// ... а если загрузка прервалась по таймеру, освобождаем ресурсы и закрываем соединение
			//
			else {
				m_isNeedRedirect = false;
				reply->abort();
			}
		}

	} while (m_isNeedRedirect);

	emit downloadComplete(m_downloadedData, m_initUrl);
}

void WebLoader::stop()
{
	if (isRunning()) {
		quit();
		wait(1000);
	}
}

void WebLoader::uploadProgress(qint64 _uploadedBytes, qint64 _totalBytes)
{
	//! отправлено [uploaded] байт из [total]
	if (_totalBytes > 0)
		emit uploadProgress(((float)_uploadedBytes / _totalBytes) * 100, m_initUrl);
}

void WebLoader::downloadProgress(qint64 _recievedBytes, qint64 _totalBytes)
{
	//! загружено [recieved] байт из [total]
	// не все сайты передают суммарный размер загружаемой страницы,
	// поэтому для отображения прогресса загрузки используется
	// заранее заданное число (средний размер веб-страницы)
	if (_totalBytes < 0)
		_totalBytes = POSSIBLE_RECIEVED_MAX_FILE_SIZE;
	emit downloadProgress(((float)_recievedBytes / _totalBytes) * 100, m_initUrl);
}

void WebLoader::downloadComplete(QNetworkReply* _reply)
{
	//! Завершена загрузка страницы [m_request->url()]

	// требуется ли редирект?
	if (!_reply->header(QNetworkRequest::LocationHeader).isNull()) {
		//! Осуществляется редирект по ссылке [redirectUrl]
		// Referer'ом становится ссылка по хоторой был осуществлен запрос
		QUrl refererUrl = m_request->urlToLoad();
		m_request->setUrlReferer(refererUrl);
		// Получаем ссылку для загрузки из заголовка ответа [Loacation]
		QUrl redirectUrl = _reply->header(QNetworkRequest::LocationHeader).toUrl();
		m_request->setUrlToLoad(redirectUrl);
		setRequestMethod(NetworkRequest::Get); // Редирект всегда методом Get
		m_isNeedRedirect = true;
	} else {
		//! Загружены данные [reply->bytesAvailable()]
		qint64 downloadedDataSize = _reply->bytesAvailable();
		QByteArray downloadedData = _reply->read(downloadedDataSize);
		m_downloadedData = downloadedData;
		_reply->deleteLater();
		m_isNeedRedirect = false;
	}

	if (!isRunning()) {
		wait(1000);
	}
	quit(); // прерываем цикл обработки событий потока (возвращаемся в run())
}

void WebLoader::downloadError(QNetworkReply::NetworkError _networkError)
{
	switch (_networkError) {

		case QNetworkReply::NoError:
			m_lastError.clear();
			m_lastErrorDetails.clear();
			break;
		default:
			m_lastError =
                    tr("%1")
					.arg(::networkErrorToString(_networkError));
			emit error(m_lastError, m_initUrl);
			break;

	}
}

void WebLoader::downloadSslErrors(const QList<QSslError>& _errors)
{
	QString fullError;
	foreach (const QSslError& error, _errors) {
		if (!fullError.isEmpty()) {
			fullError.append("\n");
		}
		fullError.append(error.errorString());
	}

	m_lastErrorDetails = fullError;
	emit errorDetails(m_lastErrorDetails, m_initUrl);
}


//*****************************************************************************
// Методы доступа к данным класса, а так же вспомогательные
// методы для работы с данными класса

void WebLoader::initNetworkManager()
{
	//
	// Создаём загрузчика, если нужно
	//
	if (m_networkManager == 0) {
		m_networkManager = new QNetworkAccessManager;
	}

	//
	// Настраиваем куки
	//
	if (m_cookieJar != 0) {
		m_networkManager->setCookieJar(m_cookieJar);
		m_cookieJar->setParent(0);
	}

	//
	// Оключаем от предыдущих соединений
	//
	m_networkManager->disconnect();
	//
	// Настраиваем новое соединение
	//
	connect(m_networkManager, &QNetworkAccessManager::finished,
			this, static_cast<void (WebLoader::*)(QNetworkReply*)>(&WebLoader::downloadComplete));
}
