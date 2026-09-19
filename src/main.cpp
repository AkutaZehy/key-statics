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
#include <QApplication>
#include <QMessageBox>
#include <QTcpServer>
#include <QHostAddress>
#include <QProcess>
#include <QDebug>
#include "mainwindow.h"
#include "config.h"

static bool tryBindPort(const QHostAddress& address, quint16 port) {
    QTcpServer probe;
    if (probe.listen(address, port)) {
        probe.close();
        return true;
    }
    return false;
}

static QString findPortOwnerProcess(quint16 port) {
#ifdef _WIN32
    QProcess process;
    process.start("netstat", QStringList() << "-ano");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList lines = output.split("\n");

    QString portStr = QString(":%1").arg(port);
    for (const QString& line : lines) {
        if (line.contains(portStr) && line.contains("LISTENING")) {
            QStringList parts = line.simplified().split(" ");
            if (parts.size() >= 5) {
                QString pid = parts.last();
                QProcess pidProcess;
                pidProcess.start("tasklist", QStringList() << "/FI" << QString("PID eq %1").arg(pid) << "/FO" << "CSV" << "/NH");
                pidProcess.waitForFinished();
                QString pidOutput = pidProcess.readAllStandardOutput();

                QString processName = pidOutput.section(",", 0, 0).remove("\"");
                if (processName.isEmpty()) {
                    processName = QString("PID: %1").arg(pid);
                }

                return QString("%1 (PID: %2)").arg(processName, pid);
            }
        }
    }
#endif
    return QString();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("key-statics");
    app.setQuitOnLastWindowClosed(false);

    Config* config = Config::instance();
    config->load();

    const QHostAddress bindAddress = config->allowRemoteAccess()
        ? QHostAddress(QHostAddress::Any)
        : QHostAddress(QHostAddress::LocalHost);

    quint16 port = config->serverPort();

    if (!tryBindPort(bindAddress, port)) {
        QString processInfo = findPortOwnerProcess(port);

        if (processInfo.contains("key-statics", Qt::CaseInsensitive)) {
            QMessageBox::critical(nullptr, "Error",
                QString("<h3>key-statics is already running</h3>"
                        "<p>Port %1 is occupied by another key-statics instance.</p>"
                        "<p>Multi-instance is not supported. Please close the existing instance.</p>"
                        "<hr><p>%2</p>").arg(port).arg(processInfo));
            return 1;
        }

        if (!config->autoPortIfOccupied()) {
            QMessageBox::critical(nullptr, "Port Error",
                QString("<h3>Port occupied</h3>"
                        "<p>Port %1 is occupied by another program.</p>"
                        "<p>Please close the program using this port, or set "
                        "<code>server.autoPortIfOccupied</code> to <code>true</code> in config.json.</p>"
                        "<hr><p><b>Process:</b> %2</p>").arg(port).arg(processInfo.isEmpty() ? "Unknown" : processInfo));
            return 1;
        }

        quint16 fallbackPort = 0;
        for (int candidate = port + 1; candidate <= port + 100; ++candidate) {
            if (tryBindPort(bindAddress, static_cast<quint16>(candidate))) {
                fallbackPort = static_cast<quint16>(candidate);
                break;
            }
        }

        if (fallbackPort == 0) {
            QMessageBox::critical(nullptr, "Port Error",
                QString("<h3>Port occupied</h3>"
                        "<p>Port %1 is occupied and no free port was found in range %2-%3.</p>"
                        "<hr><p><b>Process:</b> %4</p>")
                    .arg(port).arg(port + 1).arg(port + 100)
                    .arg(processInfo.isEmpty() ? "Unknown" : processInfo));
            return 1;
        }

        QMessageBox::information(nullptr, "Port Changed",
            QString("<h3>Port %1 is occupied</h3>"
                    "<p>key-statics will use port <b>%2</b> instead.</p>"
                    "<p>Update your OBS Browser Source URL to "
                    "<code>http://localhost:%2/</code></p>"
                    "<hr><p><b>Occupied by:</b> %3</p>")
                .arg(port).arg(fallbackPort)
                .arg(processInfo.isEmpty() ? "Unknown" : processInfo));

        config->setServerPort(fallbackPort);
        port = fallbackPort;
    }

    MainWindow window;
    window.hide();

    return app.exec();
}
