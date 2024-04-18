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
    setHeaders(&request, "cookies");

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

    QNetworkRequest request(url);
    setHeaders(&request, "token");

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
        request->setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");
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
        request->setRawHeader("origin", "https://chat.openai.com");
        request->setRawHeader("referer", "https://chat.openai.com/");
        request->setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36");
        if (type == "response") {
            request->setRawHeader("openai-sentinel-chat-requirements-token", m_token.toUtf8());
        }
    }
}

QString GPT::getResponse(QString prompt)
{
    QUrl url = QUrl(m_baseUrl + QString("/backend-anon/conversation"));

    QNetworkRequest request(url);
    setHeaders(&request, "response");

    QJsonDocument jDoc { getPromptJson(prompt) };
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
