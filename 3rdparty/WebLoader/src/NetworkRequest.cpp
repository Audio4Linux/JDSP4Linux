/*
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

#include "NetworkRequest.h"
#include "NetworkQueue_p.h"
#include "NetworkRequestPrivate_p.h"
#include "WebLoader_p.h"
#include "WebRequest_p.h"

NetworkRequestPrivate::NetworkRequestPrivate(QObject* _parent, CustomCookieJar* _jar)
    : QObject(_parent), m_cookieJar(_jar), m_loadingTimeout(20000), m_request(new WebRequest())

{

}

NetworkRequestPrivate::~NetworkRequestPrivate()
{
    delete m_request;
}

void NetworkRequestPrivate::done()
{
    emit finished();
}

NetworkRequest::NetworkRequest(QObject* _parent, CustomCookieJar* _jar)
    : QObject(_parent), m_internal(new NetworkRequestPrivate(this, _jar))
{
    //
    // Соединим сигналы от internal с сигналами/слотами этого класса
    //
    connect(m_internal, &NetworkRequestPrivate::downloadComplete, this, &NetworkRequest::downloadComplete);
    connect(m_internal, &NetworkRequestPrivate::downloadProgress,
            this, &NetworkRequest::downloadProgress);
    connect(m_internal, &NetworkRequestPrivate::uploadProgress,
            this, &NetworkRequest::uploadProgress);
    connect(m_internal, &NetworkRequestPrivate::error,
            this, &NetworkRequest::slotError);
    connect(m_internal, &NetworkRequestPrivate::errorDetails,
            this, &NetworkRequest::slotErrorDetails);
    connect(m_internal, &NetworkRequestPrivate::finished,
            this, &NetworkRequest::finished);

}

NetworkRequest::~NetworkRequest()
{

}

void NetworkRequest::setCookieJar(CustomCookieJar* _cookieJar)
{
    stop();
    m_internal->m_cookieJar = _cookieJar;
}

CustomCookieJar* NetworkRequest::getCookieJar()
{
    return m_internal->m_cookieJar;
}

void NetworkRequest::setRequestMethod(NetworkRequest::RequestMethod _method)
{
    stop();
    m_internal->m_method = _method;
}

NetworkRequest::RequestMethod NetworkRequest::getRequestMethod() const
{
    return m_internal->m_method;
}

void NetworkRequest::setLoadingTimeout(int _loadingTimeout)
{
    stop();
    m_internal->m_loadingTimeout = _loadingTimeout;
}

int NetworkRequest::getLoadingTimeout() const
{
    return m_internal->m_loadingTimeout;
}

void NetworkRequest::clearRequestAttributes()
{
    stop();
    m_internal->m_request->clearAttributes();
}

void NetworkRequest::addRequestAttribute(const QString& _name, const QVariant& _value)
{
    stop();
    m_internal->m_request->addAttribute(_name, _value);
}

void NetworkRequest::addRequestAttributeFile(const QString& _name, const QString& _filePath)
{
    stop();
    m_internal->m_request->addAttribute(_name, _filePath);
}

void NetworkRequest::setRawRequest(const QByteArray &_data)
{
    stop();
    m_internal->m_request->setRawRequest(_data);
}

void NetworkRequest::setRawRequest(const QByteArray &_data, const QString &_mime)
{
    stop();
    m_internal->m_request->setRawRequest(_data, _mime);
}

void NetworkRequest::loadAsync(const QString& _urlToLoad, const QUrl& _referer)
{
    loadAsync(QUrl(_urlToLoad), _referer);
}

void NetworkRequest::loadAsync(const QUrl& _urlToLoad, const QUrl& _referer)
{
    //
    // Получаем инстанс очереди
    //
    NetworkQueue* nq = NetworkQueue::getInstance();

    //
    // Останавливаем в очереди запрос, связанный с данным классом (если имеется)
    //
    nq->stop(m_internal);

    //
    // Настраиваем параметры и кладем в очередь
    m_internal->m_request->setUrlToLoad(_urlToLoad);
    m_internal->m_request->setUrlReferer(_referer);
    nq->put(m_internal);
}

QByteArray NetworkRequest::loadSync(const QString& _urlToLoad, const QUrl& _referer)
{
    return loadSync(QUrl(_urlToLoad), _referer);
}

QByteArray NetworkRequest::loadSync(const QUrl& _urlToLoad, const QUrl& _referer)
{
    //
    // Для синхронного запроса используем QEventLoop и асинхронный запрос
    //
    QEventLoop loop;
    connect(this, &NetworkRequest::finished, &loop, &QEventLoop::quit);
    connect(m_internal, static_cast<void (NetworkRequestPrivate::*)(QByteArray, QUrl)>(&NetworkRequestPrivate::downloadComplete),
            this, &NetworkRequest::downloadCompleteData);
    loadAsync(_urlToLoad, _referer);
    loop.exec();

    return m_downloadedData;
}

QUrl NetworkRequest::url() const
{
    return m_internal->m_request->urlToLoad();
}

void NetworkRequest::downloadCompleteData(const QByteArray& _data)
{
    m_downloadedData = _data;
}

void NetworkRequest::slotError(const QString& _errorStr, const QUrl& _url)
{
    m_lastError = _errorStr;
    emit error(_errorStr, _url);
}

void NetworkRequest::slotErrorDetails(const QString& _errorStr)
{
    m_lastErrorDetails = _errorStr;
}

QString NetworkRequest::lastError() const
{
    return m_lastError;
}

void NetworkRequest::stop()
{
    NetworkQueue* nq = NetworkQueue::getInstance();
    nq->stop(m_internal);
}

QString NetworkRequest::lastErrorDetails() const
{
    return m_lastErrorDetails;
}
