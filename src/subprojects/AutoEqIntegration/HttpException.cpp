#include "HttpException.h"

HttpException::HttpException(const HttpReply& reply)
{
    _statusCode = reply.statusCode();
    _reasonPhrase = reply.reasonPhrase();
}

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
