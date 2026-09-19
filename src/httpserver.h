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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QTimer>
#include "keystats.h"
#include "keylayout.h"

class HttpServer : public QObject {
    Q_OBJECT

public:
    explicit HttpServer(KeyStats* stats, QObject* parent = nullptr);
    ~HttpServer();

    bool start(quint16 port = 9876);
    void stop();
    void setLayout(KeyLayout* layout);
    // Pushes a named "reload" SSE event so connected browser sources refetch
    // the page after the layout changed on disk.
    void notifyLayoutChanged();

private slots:
    void onNewConnection();
    void onStatsChanged();

private:
    void handleRequest(QTcpSocket* socket, const QString& request);
    void sendHtml(QTcpSocket* socket);
    void sendJson(QTcpSocket* socket);
    void sendSse(QTcpSocket* socket);
    void broadcastSse();
    void sendNotFound(QTcpSocket* socket);
    void writeResponse(QTcpSocket* socket, const QString& response);
    QString ssePayload() const;
    QString generateKeyboardJson() const;

    QTcpServer* m_server = nullptr;
    KeyStats* m_stats = nullptr;
    KeyLayout* m_layout = nullptr;
    quint16 m_port = 9876;
    QList<QTcpSocket*> m_sseClients;
    QHash<QTcpSocket*, QByteArray> m_requestBuffers;
    QTimer* m_sseCoalesceTimer = nullptr;
};

#endif
