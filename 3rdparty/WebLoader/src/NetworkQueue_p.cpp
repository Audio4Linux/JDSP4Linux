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

#include "NetworkQueue_p.h"
#include "WebLoader_p.h"
#include "WebRequest_p.h"
#include "NetworkRequestPrivate_p.h"

NetworkQueue::NetworkQueue()
{
    //
    // В нужном количестве создадим WebLoader'ы
    // И сразу же соединим их со слотом данного класса, обозначающим завершение
    //
    for (int i = 0; i != qMax(QThread::idealThreadCount(), 4); ++i) {
        m_freeLoaders.push_back(new WebLoader(this));
        connect(m_freeLoaders.back(), &WebLoader::finished,
                this, static_cast<void (NetworkQueue::*)()>(&NetworkQueue::downloadComplete));
    }
}

NetworkQueue* NetworkQueue::getInstance() {
    static NetworkQueue queue;
    return &queue;
}

void NetworkQueue::put(NetworkRequestPrivate* _request) {
    //
    // Положим в очередь пришедший запрос
    //
    m_queue.push_back(_request);
    m_inQueue.insert(_request);

    //
    // В случае, если есть свободный WebLoader
    // Извлечем пришедший запрос из очереди и начнем выполнять его
    //
    if (!m_freeLoaders.empty()) {
        pop();
    }
}

void NetworkQueue::pop() {
    //
    // Извлечем свободный WebLoader
    //
    WebLoader* loader = m_freeLoaders.front();
    m_freeLoaders.pop_front();

    //
    // Извлечем первый запрос на обработку
    //
    NetworkRequestPrivate *request = m_queue.front();
    m_queue.pop_front();
    m_inQueue.remove(request);

    //
    // Настроим WebLoader на запрос
    //
    m_busyLoaders[loader] = request;
    setLoaderParams(loader, request);

    //
    // Соединим сигналы WebLoader'а с сигналами класса запроса
    //
    connect(loader, static_cast<void (WebLoader::*)(QByteArray, QUrl)>(&WebLoader::downloadComplete),
            request, &NetworkRequestPrivate::downloadComplete);
    connect(loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::uploadProgress),
            request, &NetworkRequestPrivate::uploadProgress);
    connect(loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::downloadProgress),
            request, &NetworkRequestPrivate::downloadProgress);
    connect(loader, &WebLoader::error, request, &NetworkRequestPrivate::error);
    connect(loader, &WebLoader::errorDetails, request, &NetworkRequestPrivate::errorDetails);

    //
    // Загружаем!
    //
    loader->loadAsync(request->m_request->urlToLoad(), request->m_request->urlReferer());
}

void NetworkQueue::stop(NetworkRequestPrivate* _internal) {
    if (m_inQueue.contains(_internal)) {
        //
        // Либо запрос еще в очереди
        // Тогда его нужно оттуда удалить
        //
        m_queue.removeAll(_internal);
        m_inQueue.remove(_internal);
    }
    else {
        //
        // Либо запрос уже обрабатывается
        //
        for (auto iter = m_busyLoaders.begin(); iter != m_busyLoaders.end(); ++iter) {
            //
            // Найдем запрос в списке обрабатывающихся
            //
            if (iter.value() == _internal) {

                //
                // Отключим все сигналы
                // Обязательно сначала отключить сигналы, а затем остановить. Не наоборот!
                //
                disconnectLoaderRequest(iter.key(), iter.value());

                //
                // Остановим запрос
                //
                iter.key()->stop();

                iter.value()->done();
                //
                // Удалим из списка используемых
                // К списку свободных WebLoader'ов припишет слот downloadComplete
                //
                m_busyLoaders.erase(iter);

                break;
            }
        }
    }
}

void NetworkQueue::setLoaderParams(WebLoader* _loader, NetworkRequestPrivate* request)
{
    _loader->setCookieJar(request->m_cookieJar);
    _loader->setRequestMethod(request->m_method);
    _loader->setLoadingTimeout(request->m_loadingTimeout);
    _loader->setWebRequest(request->m_request);
}

void NetworkQueue::disconnectLoaderRequest(WebLoader* _loader,
                                           NetworkRequestPrivate* _request)
{
    disconnect(_loader, static_cast<void (WebLoader::*)(QByteArray, QUrl)>(&WebLoader::downloadComplete),
               _request, &NetworkRequestPrivate::downloadComplete);
    disconnect(_loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::uploadProgress),
            _request, &NetworkRequestPrivate::uploadProgress);
    disconnect(_loader, static_cast<void (WebLoader::*)(int, QUrl)>(&WebLoader::downloadProgress),
            _request, &NetworkRequestPrivate::downloadProgress);
    disconnect(_loader, &WebLoader::error, _request, &NetworkRequestPrivate::error);
    disconnect(_loader, &WebLoader::errorDetails, _request, &NetworkRequestPrivate::errorDetails);
}

void NetworkQueue::downloadComplete()
{
    WebLoader* loader = qobject_cast<WebLoader*>(sender());
    if (m_busyLoaders.contains(loader)) {
        //
        // Если запрос отработал до конца (не был прерван методом stop),
        // то необходимо отключить сигналы
        // и удалить из списка используемых
        //
        NetworkRequestPrivate* request = m_busyLoaders[loader];

        disconnectLoaderRequest(loader, request);
        request->done();

        m_busyLoaders.remove(loader);
    }

    //
    // Добавляем WebLoader в список свободных
    //
    m_freeLoaders.push_back(loader);

    //
    //Смотрим, надо ли что еще выполнить из очереди
    //
    if (!m_queue.empty()) {
        pop();
    }
}
