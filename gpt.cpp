#include "gpt.h"

GPT::GPT(QObject *parent)
    : QObject{parent}
{
    m_manager->setCookieJar(m_cJar);
    m_manager->setTransferTimeout(5000);
    if (!getCookies())
        throw AccessException();
}


bool GPT::getCookies()
{
    QUrl url = QUrl(m_baseUrl);

    QNetworkRequest request(url);
    setHeaders(&request, "cookies");

    QNetworkReply * reply = m_manager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200)
        return true;
    return false;
}

bool GPT::getChatRequirements()
{
    QUrl url = QUrl(m_baseUrl + QString("/backend-anon/sentinel/chat-requirements"));

    QNetworkRequest request(url);
    setHeaders(&request, "token");

    QNetworkReply * reply = m_manager->post(request, QByteArray(""));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        m_token = json.value("token").toString().toUtf8();
        QJsonObject powObject = json.value("proofofwork").toObject();
        if (powObject.value("required") == true
            and !getPowToken(powObject.value("seed").toString(), powObject.value("difficulty").toString(), m_userAgent))
                return false;
        return true;
    }
    return false;
}

bool GPT::getPowToken(QString seed, QString difficulty, QString userAgent)
{
    QByteArray response = "gAAAAAB";
    int wh = QRandomGenerator::global()->bounded(1000, 3001);
    for (int attempt = 0; attempt < 100000; ++attempt) {
        QJsonArray config {
            wh,
            QDateTime::currentDateTimeUtc().toString(),
            QJsonValue(),
            attempt,
            userAgent
        };

        QJsonDocument doc;
        doc.setArray(config);

        QByteArray configString(doc.toJson(QJsonDocument::Compact).toBase64());
        QByteArray hash = QCryptographicHash::hash(seed.toUtf8() + configString, QCryptographicHash::Algorithm::Sha3_512).toHex();

        hash.truncate(difficulty.length());

        if (hash <= difficulty) {
            response.append(configString);
            m_powToken = response;
            return true;
        }
    }
    return false;
}

QJsonObject GPT::getPromptJson(QString prompt)
{
    return {
        {"action", "next"},
        {"messages", QJsonArray {
                            QJsonObject {
                                { "id", QUuid::createUuid().toString() },
                                { "author",  QJsonObject {
                                                  { "role", "user" }
                                              }
                                },
                                { "content", QJsonObject {
                                                   { "content_type", "text" },
                                                   { "parts", QJsonArray { prompt } }
                                               }
                                },
                                { "metadata", QJsonObject {} }
                            }
                        }
        },

        {"parent_message_id", QUuid::createUuid().toString()},
        {"model", "text-davinci-002-render-sha"},
        {"timezone_offset_min", -330},
        {"history_and_training_disabled", true},
        {"conversation_mode", QJsonObject {
                { "kind", "primary_assistant" }
            }
        },
        {"force_paragen", false},
        {"force_paragen_model_slug", ""},
        {"force_nulligen", false},
        {"force_rate_limit", false}
    };
}

void GPT::setHeaders(QNetworkRequest *request, QString type)
{
    if (type == "cookies") {
        request->setRawHeader("User-Agent", m_userAgent.toUtf8());
    } else if (type == "token" || type == "response") {
        QByteArray deviceId;

        QList<QNetworkCookie> cookies = m_cJar->cookiesForUrl(QUrl(m_baseUrl));

        for (int cookie = 0; cookie < cookies.length(); ++cookie)
            if (cookies[cookie].name() == "oai-did") {
                deviceId = cookies[cookie].value();
                break;
            }

        request->setRawHeader("Accept", "text/event-stream");
        request->setRawHeader("Accept-Language", "en-GB,en;q=0.8");
        request->setRawHeader("oai-device-id", deviceId);
        request->setRawHeader("oai-language", "en-US");
        request->setRawHeader("origin", m_baseUrl.toUtf8());
        request->setRawHeader("referer", m_baseUrl.toUtf8());
        request->setRawHeader("User-Agent", m_userAgent.toUtf8());
        if (type == "response") {
            request->setRawHeader("openai-sentinel-chat-requirements-token", m_token);
            if (m_powToken.length()) {
                request->setRawHeader("OpenAI-Sentinel-Proof-Token", m_powToken);
            }
        }
    }
}

QString GPT::getResponse(QString prompt)
{
    if (!getChatRequirements())
        throw AccessException();

    QUrl url = QUrl(m_baseUrl + QString("/backend-anon/conversation"));

    QNetworkRequest request(url);
    setHeaders(&request, "response");

    QJsonDocument jDoc { getPromptJson(prompt) };
    QByteArray data = jDoc.toJson();

    QNetworkReply * reply = m_manager->post(request, data);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    loop.exec();

    if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        QList responseList = QString(reply->readAll()).split("data: ");
        QString lastResponse = responseList.at(responseList.size() - 2);
        QString move = QJsonDocument::fromJson(lastResponse.toUtf8()).object()
                    .value("message").toObject()
                    .value("content").toObject()
                    .value("parts").toArray()
                    .first().toString();

        return move;
    }
    throw AccessException();
}
