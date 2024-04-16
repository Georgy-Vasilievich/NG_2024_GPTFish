#include "gpt.h"

GPT::GPT(QObject *parent)
    : QObject{parent}
{
    m_manager->setCookieJar(m_cJar);
    if (!getCookies() || !getToken())
        throw AccessException();
}


bool GPT::getCookies()
{
    QUrl url = QUrl(m_baseUrl);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");

    QNetworkReply * reply = m_manager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200)
        return true;
    return false;
}

bool GPT::getToken()
{
    QUrl url = QUrl(m_baseUrl + QString("/backend-anon/sentinel/chat-requirements"));

    QByteArray deviceId;

    for (auto &it : m_cJar->cookiesForUrl(QUrl(m_baseUrl)))
        if (it.name() == "oai-did") {
            deviceId = it.value();
            break;
        }

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Accept-Language", "en-GB,en;q=0.8");
    request.setRawHeader("oai-device-id", deviceId);
    request.setRawHeader("oai-language", "en-US");
    request.setRawHeader("origin", "https://chat.openai.com");
    request.setRawHeader("referer", "https://chat.openai.com/");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");

    QNetworkReply * reply = m_manager->post(request, QByteArray(""));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        m_token = json.value("token").toString();
        return true;
    }
    return false;
}

QString GPT::getResponse(QString prompt)
{
    QUrl url = QUrl(m_baseUrl + QString("/backend-anon/conversation"));

    QByteArray deviceId;

    for (auto &it : m_cJar->cookiesForUrl(QUrl(m_baseUrl)))
        if (it.name() == "oai-did") {
            deviceId = it.value();
            break;
        }

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "text/event-stream");
    request.setRawHeader("Accept-Language", "en-GB,en;q=0.8");
    request.setRawHeader("oai-device-id", deviceId);
    request.setRawHeader("oai-language", "en-US");
    request.setRawHeader("origin", "https://chat.openai.com");
    request.setRawHeader("referer", "https://chat.openai.com/");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");
    request.setRawHeader("openai-sentinel-chat-requirements-token", m_token.toUtf8());

    QJsonObject json {
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

    QJsonDocument jDoc { json };
    QByteArray data = jDoc.toJson();

    QNetworkReply * reply = m_manager->post(request, data);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
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
