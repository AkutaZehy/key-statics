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
#include "inputconsts.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

HttpServer::HttpServer(KeyStats* stats, QObject* parent)
    : QObject(parent)
    , m_stats(stats)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);

    // Coalesce bursts of statsUpdated into at most one SSE frame per tick.
    m_sseCoalesceTimer = new QTimer(this);
    m_sseCoalesceTimer->setSingleShot(true);
    m_sseCoalesceTimer->setInterval(8);
    connect(m_sseCoalesceTimer, &QTimer::timeout, this, &HttpServer::broadcastSse);

    if (m_stats) {
        connect(m_stats, &KeyStats::statsUpdated, this, &HttpServer::onStatsChanged);
    }
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
    const QHostAddress bindAddress = Config::instance()->allowRemoteAccess()
        ? QHostAddress(QHostAddress::Any)
        : QHostAddress(QHostAddress::LocalHost);

    if (m_server->listen(bindAddress, m_port)) {
        qDebug() << "HTTP server started on"
                 << (bindAddress == QHostAddress::LocalHost ? "127.0.0.1" : "0.0.0.0")
                 << ":" << m_port;
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
    for (QTcpSocket* client : m_sseClients) {
        client->disconnectFromHost();
    }
    m_sseClients.clear();
    m_requestBuffers.clear();
}

void HttpServer::onNewConnection() {
    QTcpSocket* socket = m_server->nextPendingConnection();
    if (!socket) return;
    socket->setParent(this);

    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        m_sseClients.removeAll(socket);
        m_requestBuffers.remove(socket);
        socket->deleteLater();
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [socket](QAbstractSocket::SocketError) {
        socket->disconnectFromHost();
    });

    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QByteArray& buffer = m_requestBuffers[socket];
        buffer += socket->readAll();

        // Wait for the full request head before parsing; drop abusive clients.
        int headEnd = buffer.indexOf("\r\n\r\n");
        if (headEnd < 0) {
            if (buffer.size() > 16 * 1024) {
                qWarning() << "Dropping client: oversized HTTP request";
                m_requestBuffers.remove(socket);
                socket->disconnectFromHost();
            }
            return;
        }
        m_requestBuffers.remove(socket);
        handleRequest(socket, QString::fromUtf8(buffer.left(headEnd)));
    });
}

void HttpServer::handleRequest(QTcpSocket* socket, const QString& request) {
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        socket->disconnectFromHost();
        return;
    }

    QString firstLine = lines.first();
    QStringList parts = firstLine.split(" ");

    if (parts.size() < 2) {
        socket->disconnectFromHost();
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
        .gauge {
            position: absolute;
            background: )" + config->keyColor() + R"(;
            border: 1px solid #555;
            border-radius: 6px;
            box-sizing: border-box;
        }
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
        const elementVk = )" + QString::number(VK_ELEMENT_FIRST) + R"(;
        const gaugeMaxSpeed = )" + QString::number(config->gaugeMaxSpeed()) + R"(;
        const activeColor = ')" + config->keyActiveColor() + R"(';

        function applyLayoutSize() {
            let cols = 0, rows = 0;
            keys.forEach(k => {
                cols = Math.max(cols, k.c + (k.w || 1));
                rows = Math.max(rows, k.r + (k.h || 1));
            });
            const kb = document.getElementById('keyboard');
            kb.style.width = (cols * unitWidth + Math.max(0, cols - 1) * keySpacing) + 'px';
            kb.style.height = (rows * unitHeight + Math.max(0, rows - 1) * keySpacing) + 'px';
        }

        function keyRect(k) {
            return {
                x: k.c * (unitWidth + keySpacing),
                y: k.r * (unitHeight + keySpacing),
                w: k.w * unitWidth + (k.w - 1) * keySpacing,
                h: (k.h || 1) * unitHeight + (k.h - 1 || 0) * keySpacing
            };
        }

        function renderKeyboard() {
            const kb = document.getElementById('keyboard');
            keys.forEach(k => {
                if (k.vk >= elementVk) {
                    renderElement(k, kb);
                    return;
                }
                const r = keyRect(k);
                const keyDiv = document.createElement('div');
                keyDiv.className = 'key';
                keyDiv.style.left = r.x + 'px';
                keyDiv.style.top = r.y + 'px';
                keyDiv.style.width = r.w + 'px';
                keyDiv.style.height = r.h + 'px';
                keyDiv.textContent = k.l || '';
                keyDiv.dataset.vk = k.vk || 0;
                kb.appendChild(keyDiv);
            });
            if (gauges.length > 0) {
                requestAnimationFrame(tickGauges);
            }
        }

        const gauges = [];

        function renderElement(k, kb) {
            const r = keyRect(k);
            const div = document.createElement('div');
            div.className = 'gauge';
            div.style.left = r.x + 'px';
            div.style.top = r.y + 'px';
            div.style.width = r.w + 'px';
            div.style.height = r.h + 'px';
            const cv = document.createElement('canvas');
            const dpr = window.devicePixelRatio || 1;
            cv.width = r.w * dpr;
            cv.height = r.h * dpr;
            cv.style.width = r.w + 'px';
            cv.style.height = r.h + 'px';
            div.appendChild(cv);
            kb.appendChild(div);
            const ctx = cv.getContext('2d');
            ctx.scale(dpr, dpr);
            const base = { ctx, w: r.w, h: r.h };
            if (k.vk === 513) {
                gauges.push({ ...base, kind: 'triggers', lt: 0, rt: 0 });
            } else {
                gauges.push({ ...base, kind: 'vector', tx: 0, ty: 0, px: 0, py: 0, vx: 0, vy: 0 });
            }
        }

        // Under-damped spring: the dot accelerates toward the measured
        // velocity vector and wobbles back to center when the mouse stops.
        function tickGauges() {
            for (const g of gauges) {
                if (g.kind === 'vector') {
                    g.vx += (g.tx - g.px) * 0.12;
                    g.vx *= 0.80;
                    g.px += g.vx;
                    g.vy += (g.ty - g.py) * 0.12;
                    g.vy *= 0.80;
                    g.py += g.vy;
                    drawGauge(g);
                } else {
                    drawTriggers(g);
                }
            }
            requestAnimationFrame(tickGauges);
        }

        function drawGauge(g) {
            const ctx = g.ctx;
            const cx = g.w / 2, cy = g.h / 2;
            const maxR = Math.min(g.w, g.h) / 2 - 8;
            ctx.clearRect(0, 0, g.w, g.h);

            ctx.strokeStyle = 'rgba(255,255,255,0.12)';
            ctx.lineWidth = 1;
            ctx.beginPath(); ctx.arc(cx, cy, maxR, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.arc(cx, cy, maxR * 0.5, 0, Math.PI * 2); ctx.stroke();

            const mag = Math.min(Math.hypot(g.px, g.py), 1.4);
            const ax = cx + g.px * maxR, ay = cy + g.py * maxR;
            const ang = Math.atan2(g.py, g.px);
            ctx.strokeStyle = activeColor;
            ctx.fillStyle = activeColor;
            ctx.lineWidth = 2;
            ctx.shadowColor = activeColor;
            ctx.shadowBlur = 4 + mag * 14;
            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.lineTo(ax, ay);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(ax, ay);
            ctx.lineTo(ax - 8 * Math.cos(ang - 0.45), ay - 8 * Math.sin(ang - 0.45));
            ctx.lineTo(ax - 8 * Math.cos(ang + 0.45), ay - 8 * Math.sin(ang + 0.45));
            ctx.closePath();
            ctx.fill();
            ctx.shadowBlur = 0;
            ctx.fillStyle = '#fff';
            ctx.beginPath(); ctx.arc(cx, cy, 2.5, 0, Math.PI * 2); ctx.fill();
        }

        function drawTriggers(g) {
            const ctx = g.ctx;
            ctx.clearRect(0, 0, g.w, g.h);
            const pad = 10;
            const barW = (g.w - pad * 3) / 2;
            const trackH = g.h - pad * 2 - 14;
            const bars = [['LT', g.lt], ['RT', g.rt]];
            for (let i = 0; i < 2; i++) {
                const x = pad + i * (barW + pad);
                ctx.fillStyle = 'rgba(0,0,0,0.35)';
                ctx.fillRect(x, pad, barW, trackH);
                const fill = Math.max(0, Math.min(255, bars[i][1])) / 255 * trackH;
                if (fill > 0) {
                    ctx.fillStyle = activeColor;
                    ctx.fillRect(x, pad + trackH - fill, barW, fill);
                }
                ctx.fillStyle = '#fff';
                ctx.font = '11px ' + 'monospace';
                ctx.textAlign = 'center';
                ctx.fillText(bars[i][0], x + barW / 2, pad + trackH + 12);
            }
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

            if (data.mvx !== undefined && data.mvy !== undefined) {
                const tx = Math.max(-1, Math.min(1, data.mvx / gaugeMaxSpeed));
                const ty = Math.max(-1, Math.min(1, data.mvy / gaugeMaxSpeed));
                for (const g of gauges) {
                    if (g.kind === 'vector') {
                        g.tx = tx;
                        g.ty = ty;
                    }
                }
            }
            if (data.pad) {
                for (const g of gauges) {
                    if (g.kind === 'triggers') {
                        g.lt = data.pad.lt;
                        g.rt = data.pad.rt;
                    }
                }
            }
        }

        let es = null;
        function connect() {
            es = new EventSource('/events');
            es.onmessage = e => updateKeys(JSON.parse(e.data));
            es.addEventListener('reload', () => window.location.reload());
            es.onerror = () => {
                es.close();
                es = null;
                setTimeout(connect, 2000);
            };
        }

        applyLayoutSize();
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

    writeResponse(socket, response);
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

    writeResponse(socket, response);
}

void HttpServer::sendNotFound(QTcpSocket* socket) {
    QString response = "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 9\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += "Not Found";

    writeResponse(socket, response);
}

void HttpServer::writeResponse(QTcpSocket* socket, const QString& response) {
    socket->write(response.toUtf8());
    socket->flush();
    socket->disconnectFromHost();
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

    m_sseClients.append(socket);

    // Push the current state right away; further frames are change-driven.
    socket->write("data: " + ssePayload().toUtf8() + "\r\n\r\n");
    socket->flush();
}

void HttpServer::notifyLayoutChanged() {
    if (m_sseClients.isEmpty()) return;

    const QByteArray frame = "event: reload\r\ndata: layout\r\n\r\n";
    for (QTcpSocket* client : m_sseClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(frame);
            client->flush();
        }
    }
}

void HttpServer::onStatsChanged() {
    if (m_sseClients.isEmpty()) return;
    if (!m_sseCoalesceTimer->isActive()) {
        m_sseCoalesceTimer->start();
    }
}

void HttpServer::broadcastSse() {
    if (m_sseClients.isEmpty() || !m_stats) return;

    QByteArray data = "data: " + ssePayload().toUtf8() + "\r\n\r\n";

    for (QTcpSocket* client : m_sseClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(data);
            client->flush();
        }
    }
}

QString HttpServer::ssePayload() const {
    QJsonObject json;
    QJsonArray pressed;
    for (int vk : m_stats->pressedKeys()) {
        pressed.append(vk);
    }
    json["pressed"] = pressed;
    json["kps"] = m_stats->kps();
    json["totalKeyPresses"] = m_stats->totalKeyPresses();
    json["mvx"] = m_stats->mouseVelocityX();
    json["mvy"] = m_stats->mouseVelocityY();

    if (m_stats->gamepadConnected()) {
        QJsonObject pad;
        pad["lt"] = m_stats->padLt();
        pad["rt"] = m_stats->padRt();
        pad["lx"] = m_stats->padLx();
        pad["ly"] = m_stats->padLy();
        pad["rx"] = m_stats->padRx();
        pad["ry"] = m_stats->padRy();
        json["pad"] = pad;
    }

    QJsonDocument doc(json);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}
