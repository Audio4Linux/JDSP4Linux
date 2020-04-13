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

#ifndef NETWORKQUEUE_H
#define NETWORKQUEUE_H

#include <QObject>
#include <QSet>
#include <QMap>

class WebLoader;
class NetworkRequestPrivate;

/*!
 * \brief Класс, реализующий очередь запросов
 * Реализован как паттерн Singleton
 */
class NetworkQueue : public QObject
{
    Q_OBJECT
public:

    /*!
     * \brief Метод, возвращающий указатель на инстанс класса
     */
    static NetworkQueue* getInstance();

    /*!
     * \brief Метод, помещающий в очередь запрос
     */
    void put(NetworkRequestPrivate*);

    /*!
     * \brief Метод, останавливающий выполняющийся запрос
     * или же убирающий его из очереди
     */
    void stop(NetworkRequestPrivate*);

signals:
    /*!
     * \brief Прогресс отправки запроса на сервер
     */
    void uploadProgress(int);

    /*!
     * \brief Прогресс загрузки данных с сервера
     */
    void downloadProgress(int);

    /*!
     * \brief Данные загружены
     */
    void downloadComplete(QByteArray);
    void finished();

    /*!
     * \brief Сигнал об ошибке
     */
    void error(QString);

private slots:
    /*!
     * \brief Слот, выполняющийся после завершения выполнения запроса
     * Начинает выполнение следующего запроса в очереди
     */
    void downloadComplete();

private:
    /*!
     * \brief Приватные конструкторы и оператор присваивания
     * Для реализации паттерна Singleton
     */
    NetworkQueue();
    NetworkQueue(const NetworkQueue&);
    NetworkQueue& operator=(const NetworkQueue&);

    /*!
     * \brief Извлечение запроса из очереди и его выполнение
     */
    void pop();

    /*!
     * \brief Настройка параметров для WebLoader'а
     */
    void setLoaderParams(WebLoader* _loader, NetworkRequestPrivate* _request);

    /*!
     * \brief Отключение сигналов WebLoader'а
     * от сигналов NetworkRequestInternal
     */
    void disconnectLoaderRequest(WebLoader* _loader, NetworkRequestPrivate* _request);

    /*!
     * \brief Очередь запросов
     */
    QList<NetworkRequestPrivate*> m_queue;

    /*!
     * \brief Множество, содержащее запросы в очереди
     * Необходим для быстрого определения, находится ли запрос в очереди
     */
    QSet<NetworkRequestPrivate*> m_inQueue;

    /*!
     * \brief WebLoader'ы, выполняющие запрос и соответствующие им запросы
     */
    QMap<WebLoader*, NetworkRequestPrivate*> m_busyLoaders;

    /*!
     * \brief Список свободных WebLoader'ов
     */
    QList<WebLoader*> m_freeLoaders;
};

#endif // NETWORKQUEUE_H
