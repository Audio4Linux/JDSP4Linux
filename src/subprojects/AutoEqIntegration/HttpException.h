#ifndef HTTPEXCEPTION_H
#define HTTPEXCEPTION_H

#include <QException>

class HttpException : public QException
{
public:
    HttpException(int statusCode, const QString& reasonPhrase);

    void raise() const override { throw *this; }
    HttpException* clone() const override { return new HttpException{*this}; }

    int statusCode() const;
    QString reasonPhrase() const;

private:
    int _statusCode;
    QString _reasonPhrase;
};

#endif // HTTPEXCEPTION_H
