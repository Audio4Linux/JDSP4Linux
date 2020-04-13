#include "autoeqclient.h"

#include <WebLoader/src/NetworkRequest.h>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

QVector<QueryResult> AutoEQClient::query(QueryRequest request){
    QVector<QueryResult> results;
    NetworkRequest net_request;
    net_request.setRequestMethod(NetworkRequest::Get);

    QString status =
            QString::fromUtf8(net_request.loadSync(INDEX_RAW_URL));
    if(net_request.lastError() != ""){
        qDebug().nospace().noquote() << "An error occurred (query): " << net_request.lastError();
        return results;
    }

    QRegularExpression re(R"(\[(?<model>.*?)\]\((?<path>.*?)\)\s+by\s+(?<group>[^\s]+))");

    QString line;
    QTextStream stream(&status);
    while(stream.readLineInto(&line)){
        if(!line.trimmed().startsWith("-"))
            continue;

        QRegularExpressionMatch match = re.match(line);
        if(!match.hasMatch())
            continue;

        QString apipath = match.captured("path").replace("./",CONTENTS_API_URL"/results/");
        QString model = match.captured("model");
        QString group = match.captured("group");

        if(request.isModelFilterEnabled() &&
                !model.contains(request.getModelFilter(),Qt::CaseInsensitive))
                continue;

        if(request.isGroupFilterEnabled() &&
                !group.contains(request.getGroupFilter(),Qt::CaseInsensitive))
                continue;

        results.push_back(QueryResult(model,
                                      group,
                                      apipath));

    }
    return results;
}

HeadphoneMeasurement AutoEQClient::fetchDetails(QueryResult item){
    HeadphoneMeasurement hp(item.getModel(),
                            item.getGroup());
    NetworkRequest net_request;
    net_request.setRequestMethod(NetworkRequest::Get);

    QString status =
            QString::fromUtf8(net_request.loadSync(item.mApiPath));
    if(net_request.lastError() != ""){
        qDebug().nospace().noquote() << "An error occurred (fetchDetails): " << net_request.lastError();
        return hp;
    }

    QJsonDocument doc = QJsonDocument::fromJson(status.toUtf8());
    QJsonArray array = doc.array();
    foreach (const QJsonValue & v, array)
    {
        QJsonObject obj = v.toObject();
        QString name = obj.value("name").toString();
        QString url = obj.value("download_url").toString();
        if(name.endsWith("GraphicEQ.txt")){
            NetworkRequest net_request_eq;
            net_request_eq.setRequestMethod(NetworkRequest::Get);

            QString status =
                    QString::fromUtf8(net_request_eq.loadSync(url));
            if(net_request_eq.lastError() != ""){
                qDebug().nospace().noquote() << "An error occurred (fetchDetails/graphiceq): " << net_request_eq.lastError();
            }
            hp.setGraphicEQ(status);
        }
        else if(name.endsWith(".png"))
            hp.setGraphUrl(url);
    }
    return hp;
}
