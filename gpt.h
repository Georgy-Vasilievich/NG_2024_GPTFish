#ifndef GPT_H
#define GPT_H

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <QUuid>
#include "accessexception.h"

class GPT : public QObject
{
    Q_OBJECT
public:
    explicit GPT(QObject *parent = nullptr);
    QString getResponse(QString);

private:
    bool getCookies();
    bool getToken();

    QString m_token;
    QString m_baseUrl = "https://chat.openai.com";
    QNetworkAccessManager *m_manager = new QNetworkAccessManager(this);
    QNetworkCookieJar *m_cJar = new QNetworkCookieJar(m_manager);
signals:
};

#endif // GPT_H
