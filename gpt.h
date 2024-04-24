#ifndef GPT_H
#define GPT_H

#include <QCryptographicHash>
#include <QDateTime>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QObject>
#include <QRandomGenerator>
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
    bool getChatRequirements();
    bool getPowToken(QString, QString, QString);
    QJsonObject getPromptJson(QString);
    void setHeaders(QNetworkRequest*, QString);

    QByteArray m_token;
    QByteArray m_powToken;
    QString m_baseUrl = "https://chat.openai.com";
    QString m_userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36";
    QNetworkAccessManager *m_manager = new QNetworkAccessManager(this);
    QNetworkCookieJar *m_cJar = new QNetworkCookieJar(m_manager);
signals:
};

#endif // GPT_H
