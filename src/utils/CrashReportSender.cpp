#include "CrashReportSender.h"

#if 0

#include "Log.h"

#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <HttpException.h>
#include <qtpromise/qpromise.h>

#define CRASH_API_ROOT "https://crash.timschneeberger.me/report"

QtPromise::QPromise<void> CrashReportSender::upload(const QString &logPath, const QString &dumpPath)
{
    return QtPromise::QPromise<void>{[=](
        const QtPromise::QPromiseResolve<void>& resolve,
                const QtPromise::QPromiseReject<void>& reject) {

        QFile log(logPath);
        QFile dump(dumpPath);

        if(!log.open(QFile::ReadOnly) || !dump.open(QFile::ReadOnly))
        {
            Log::error("Failed to open previous log or/and crash dump");
            reject();
            return;
        }

        QString out;
        QTextStream stream(&out);
        stream << dump.readAll();
        stream << "=== Log:" << "\n";
        stream << log.readAll();

        dump.close();
        log.close();

        QJsonObject root;
        root["content"] = QString::fromLocal8Bit(out.toUtf8().toBase64());

        auto* nam = new QNetworkAccessManager();

        auto reqProto = QNetworkRequest(QUrl(CRASH_API_ROOT));
        reqProto.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        reqProto.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);

        QtPromise::connect(nam, &QNetworkAccessManager::finished).then([=](QNetworkReply *reply)
        {
            if(reply->error() != QNetworkReply::NoError)
            {
                throw HttpException(1, reply->errorString());
            }

            QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

            if (statusCode.toInt() == 200)
            {
                Log::debug("Successfully submitted");
            }
            else
            {
                throw HttpException(statusCode.toInt(), reason.toString());
            }
        }).fail([reject](const HttpException& error) {
            Log::error("Http exception: " + error.reasonPhrase() + " (" + error.statusCode() + ")");
            reject();
        }).finally([=]{
            nam->deleteLater();
        });

        nam->post(reqProto, QJsonDocument(root).toJson(QJsonDocument::Compact));
        resolve();
    }};
}

#endif
