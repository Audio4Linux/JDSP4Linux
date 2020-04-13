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

#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include <QEventLoop>
#include <QTimer>
#include <QUrl>

class NetworkRequestPrivate;
class WebRequest;
class CustomCookieJar;

/*!
 * \brief Пользовательский класс для создания GET и POST запросов
 */

class NetworkRequest : public QObject
{
    Q_OBJECT
public:

    /*!
    \enum Метод запроса
    */
    enum RequestMethod {
        Undefined, /*!< Метод не установлен */
        Get,
        Post
    };

    explicit NetworkRequest(QObject* _parent = 0, CustomCookieJar* _jar = 0);
    virtual ~NetworkRequest();

    /*!
     * \brief Установка cookie для загрузчика
     */
    void setCookieJar(CustomCookieJar* _cookieJar);

    /*!
     * \brief Получение cookie загрузчика
     */
    CustomCookieJar* getCookieJar();

    /*!
     * \brief Установка метода запроса
     */
    void setRequestMethod(RequestMethod _method);

    /*!
     * \brief Получение метода запроса
     */
    RequestMethod getRequestMethod() const;

    /*!
     * \brief Установка таймаута загрузки
     */
    void setLoadingTimeout(int _loadingTimeout);

    /*!
     * \brief Получение таймаута загрузки
     */
    int getLoadingTimeout() const;

    /*!
     * \brief Очистить все старые атрибуты запроса
     */
    void clearRequestAttributes();

    /*!
     * \brief Добавление атрибута в запрос
     */
    void addRequestAttribute(const QString& _name, const QVariant& _value);

    /*!
     * \brief Добавление файла в запрос
     */
    void addRequestAttributeFile(const QString& _name, const QString& _filePath);

    void setRawRequest(const QByteArray& _data);
    void setRawRequest(const QByteArray& _data, const QString& _mime);

    /*!
     * \brief Асинхронная загрузка запроса
     */
    void loadAsync(const QString& _urlToLoad, const QUrl& _referer = QUrl());
    void loadAsync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());
    /*!
     * \brief Синхронная загрузка запроса
     */
    QByteArray loadSync(const QString& _urlToLoad, const QUrl& _referer = QUrl());
    QByteArray loadSync(const QUrl& _urlToLoad, const QUrl& _referer = QUrl());

    /*!
     * \brief Получение загруженного URL
     */
    QUrl url() const;

    /*!
     * \brief Получение строки с последней ошибкой
     */
    QString lastError() const;
    QString lastErrorDetails() const;

signals:
    /*!
     * \brief Прогресс отправки запроса на сервер
     */
    void uploadProgress(int, QUrl);

    /*!
     * \brief Прогресс загрузки данных с сервера
     */
    void downloadProgress(int, QUrl);

    /*!
     * \brief Данные загружены
     */
    void downloadComplete(QByteArray, QUrl);
    void finished();

    /*!
     * \brief Сигнал об ошибке
     */
    void error(QString, QUrl);

private:

    /*!
     * \brief Объект, используемый в очереди запросов
     */
    NetworkRequestPrivate *m_internal;

    /*!
     * \brief Загруженные данные в случае, если используется синхронная загрузка
     */
    QByteArray m_downloadedData;

    /*!
     * \brief Строка, содержащая описание последней ошибки
     */
    QString m_lastError;
    QString m_lastErrorDetails;

    /*!
     * \brief Остановка выполнения запроса, связанного с текущим объектом
     * и удаление запросов, ожидающих в очереди, связанных с текущим объектом
     */
    void stop();

private slots:
    /*!
     * \brief Данные загружены. Используется при синхронной загрузке
     */
    void downloadCompleteData(const QByteArray&);

    /*!
     * \brief Ошибка при получении данных
     */
    void slotError(const QString&, const QUrl&);
    void slotErrorDetails(const QString&);
};

#endif // NETWORKREQUEST_H
