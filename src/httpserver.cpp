/*
 * Copyright (C) 2026 Akuta Zehy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "httpserver.h"
#include "config.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

QList<QTcpSocket*> sseClients;

HttpServer::HttpServer(KeyStats* stats, QObject* parent)
    : QObject(parent)
    , m_stats(stats)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &HttpServer::broadcastSse);
    timer->start(16);
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setLayout(KeyLayout* layout) {
    m_layout = layout;
}

QString HttpServer::generateKeyboardJson() const {
    if (!m_layout) return "[]";
    
    QStringList keyList;
    const QMap<int, KeyInfo>& keys = m_layout->keys();
    for (auto it = keys.constBegin(); it != keys.constEnd(); ++it) {
        const KeyInfo& info = it.value();
        QString label = info.label;
        label.replace("\\", "\\\\");
        label.replace("'", "\\'");
        QString keyStr = QString("{l:'%1',vk:%2,r:%3,c:%4,w:%5")
            .arg(label)
            .arg(info.vkCode)
            .arg(info.row)
            .arg(info.col)
            .arg(info.width);
        if (info.height > 1) {
            keyStr += ",h:" + QString::number(info.height);
        }
        keyStr += "}";
        keyList.append(keyStr);
    }
    return "[" + keyList.join(",") + "]";
}

bool HttpServer::start(quint16 port) {
    m_port = port;
    if (m_server->listen(QHostAddress::Any, m_port)) {
        qDebug() << "HTTP server started on port" << m_port;
        return true;
    }
    qWarning() << "Failed to start HTTP server:" << m_server->errorString();
    return false;
}

void HttpServer::stop() {
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "HTTP server stopped";
    }
}

void HttpServer::onNewConnection() {
    QTcpSocket* socket = m_server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { onReadyRead(); });
}

void HttpServer::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);

    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) return;

    QString firstLine = lines.first();
    QStringList parts = firstLine.split(" ");

    if (parts.size() < 2) {
        socket->close();
        return;
    }

    QString path = parts[1];

    if (path == "/" || path.startsWith("/index")) {
        sendHtml(socket);
    } else if (path == "/query" || path == "/api/stats") {
        sendJson(socket);
    } else if (path == "/events" || path == "/sse") {
        sendSse(socket);
    } else {
        sendNotFound(socket);
    }
}

void HttpServer::sendHtml(QTcpSocket* socket) {
    Config* config = Config::instance();
    QString html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Key Stats</title>
    <style>
        body { background: transparent; margin: 0; padding: 10px; font-family: )" + config->fontFamily() + R"(; }
        .keyboard {
            position: relative;
            width: 1000px;
            height: 300px;
        }
        .key { 
            position: absolute;
            background: )" + config->keyColor() + R"(; 
            border: 1px solid #555; 
            border-radius: 4px; 
            padding: 4px; 
            text-align: center; 
            color: #fff;
            font-size: 11px;
            display: flex;
            align-items: center;
            justify-content: center;
            box-sizing: border-box;
            transition: background 0.1s;
        }
        .key.pressed { background: )" + config->keyActiveColor() + R"(; }
        .stats { color: #0f0; font-size: 14px; margin-bottom: 10px; }
    </style>
</head>
<body>
    <div class="stats">
        <span id="kps">KPS: 0</span> | 
        <span id="total">Total: 0</span>
    </div>
    <div class="keyboard" id="keyboard"></div>
    <script>
        const unitWidth = )" + QString::number(config->unitWidth()) + R"(;
        const unitHeight = )" + QString::number(config->unitHeight()) + R"(;
        const keySpacing = )" + QString::number(config->keySpacing()) + R"(;
        const keys = )" + generateKeyboardJson() + R"(;
        
        function renderKeyboard() {
            const kb = document.getElementById('keyboard');
            keys.forEach(k => {
                const keyDiv = document.createElement('div');
                keyDiv.className = 'key';
                const x = k.c * (unitWidth + keySpacing);
                const y = k.r * (unitHeight + keySpacing);
                const w = k.w * unitWidth + (k.w - 1) * keySpacing;
                const h = (k.h || 1) * unitHeight + (k.h - 1 || 0) * keySpacing;
                keyDiv.style.left = x + 'px';
                keyDiv.style.top = y + 'px';
                keyDiv.style.width = w + 'px';
                keyDiv.style.height = h + 'px';
                keyDiv.textContent = k.l || '';
                keyDiv.dataset.vk = k.vk || 0;
                kb.appendChild(keyDiv);
            });
        }
        
        function updateKeys(data) {
            const pressed = {};
            if (data.pressed) data.pressed.forEach(v => pressed[v] = true);
            document.querySelectorAll('.key').forEach(k => {
                const vk = parseInt(k.dataset.vk);
                k.classList.toggle('pressed', !!pressed[vk]);
            });
            
            document.getElementById('kps').textContent = 'KPS: ' + data.kps;
            document.getElementById('total').textContent = 'Total: ' + data.totalKeyPresses;
        }
        
        function connect() {
            const es = new EventSource('/events');
            es.onmessage = e => updateKeys(JSON.parse(e.data));
            es.onerror = () => es.close();
        }
        
        renderKeyboard();
        connect();
    </script>
</body>
</html>)";

    QString response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=UTF-8\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Content-Length: " + QString::number(html.toUtf8().size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += html;

    socket->write(response.toUtf8());
    socket->flush();
    socket->close();
}

void HttpServer::sendJson(QTcpSocket* socket) {
    QString response;
    if (m_stats) {
        QJsonObject json;
        json["totalKeyPresses"] = m_stats->totalKeyPresses();
        json["kps"] = m_stats->kps();
        
        QJsonObject keyCounts;
        for (auto it = m_stats->keyCounts().constBegin(); it != m_stats->keyCounts().constEnd(); ++it) {
            keyCounts[QString::number(it.key())] = it.value();
        }
        json["keyCounts"] = keyCounts;

        QJsonDocument doc(json);
        QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Content-Length: " + QString::number(jsonStr.toUtf8().size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += jsonStr;
    } else {
        response = "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: 17\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += "Stats not available";
    }

    socket->write(response.toUtf8());
    socket->flush();
    socket->close();
}

void HttpServer::sendKeys(QTcpSocket* socket) {
    QString response;
    if (m_stats) {
        QJsonObject json;
        
        QJsonArray pressed;
        for (int vk : m_stats->pressedKeys()) {
            pressed.append(vk);
        }
        json["pressed"] = pressed;
        
        QJsonObject counts;
        for (auto it = m_stats->keyCounts().constBegin(); it != m_stats->keyCounts().constEnd(); ++it) {
            counts[QString::number(it.key())] = it.value();
        }
        json["keyCounts"] = counts;
        
        json["kps"] = m_stats->kps();
        json["totalKeyPresses"] = m_stats->totalKeyPresses();

        QJsonDocument doc(json);
        QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Content-Length: " + QString::number(jsonStr.toUtf8().size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += jsonStr;
    } else {
        response = "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: 17\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += "Stats not available";
    }

    socket->write(response.toUtf8());
    socket->flush();
    socket->close();
}

void HttpServer::sendNotFound(QTcpSocket* socket) {
    QString response = "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 9\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += "Not Found";

    socket->write(response.toUtf8());
    socket->flush();
    socket->close();
}

QString HttpServer::getPressedKeysJson() const {
    if (!m_stats) return "{}";

    QJsonObject json;
    QJsonObject keyCounts;
    for (auto it = m_stats->keyCounts().constBegin(); it != m_stats->keyCounts().constEnd(); ++it) {
        keyCounts[QString::number(it.key())] = it.value();
    }
    json["keyCounts"] = keyCounts;

    QJsonDocument doc(json);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void HttpServer::sendSse(QTcpSocket* socket) {
    QString response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/event-stream\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "Connection: keep-alive\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    socket->write(response.toUtf8());
    socket->flush();
    
    sseClients.append(socket);
    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        sseClients.removeAll(socket);
    });
}

void HttpServer::broadcastSse() {
    if (sseClients.isEmpty() || !m_stats) return;
    
    QJsonObject json;
    QJsonArray pressed;
    for (int vk : m_stats->pressedKeys()) {
        pressed.append(vk);
    }
    json["pressed"] = pressed;
    json["kps"] = m_stats->kps();
    json["totalKeyPresses"] = m_stats->totalKeyPresses();
    
    QJsonObject keyCounts;
    for (auto it = m_stats->keyCounts().constBegin(); it != m_stats->keyCounts().constEnd(); ++it) {
        keyCounts[QString::number(it.key())] = it.value();
    }
    json["keyCounts"] = keyCounts;
    
    QJsonDocument doc(json);
    QString data = "data: " + QString::fromUtf8(doc.toJson(QJsonDocument::Compact)) + "\r\n\r\n";
    
    for (QTcpSocket* client : sseClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(data.toUtf8());
            client->flush();
        }
    }
}
