#include "HttpException.h"

HttpException::HttpException(int statusCode, const QString &reasonPhrase)
{
    _statusCode = statusCode;
    _reasonPhrase = reasonPhrase;
}


int HttpException::statusCode() const
{
    return _statusCode;
}

QString HttpException::reasonPhrase() const
{
    return _reasonPhrase;
}
