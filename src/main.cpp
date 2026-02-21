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

bool checkPortAndNotify(quint16 port) {
    QTcpServer testServer;
    if (testServer.listen(QHostAddress::Any, port)) {
        testServer.close();
        return true;
    }
    
    QString processInfo = "";
    
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
                
                processInfo = QString("%1 (PID: %2)").arg(processName, pid);
                break;
            }
        }
    }
#endif
    
    QString appName = "key-statics";
    if (processInfo.contains(appName, Qt::CaseInsensitive)) {
        QMessageBox::critical(nullptr, "Error",
            QString("<h3>key-statics is already running</h3>"
                    "<p>Port %1 is occupied by another key-statics instance.</p>"
                    "<p>Multi-instance is not supported. Please close the existing instance.</p>"
                    "<hr><p>%2</p>").arg(port).arg(processInfo));
    } else {
        QMessageBox::critical(nullptr, "Port Error",
            QString("<h3>Port occupied</h3>"
                    "<p>Port %1 is occupied by another program.</p>"
                    "<p>Please close the program using this port.</p>"
                    "<hr><p><b>Process:</b> %2</p>").arg(port).arg(processInfo.isEmpty() ? "Unknown" : processInfo));
    }
    
    return false;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("key-statics");
    app.setQuitOnLastWindowClosed(false);
    
    Config::instance()->load();
    quint16 port = Config::instance()->serverPort();
    
    if (!checkPortAndNotify(port)) {
        return 1;
    }
    
    MainWindow window;
    window.hide();

    return app.exec();
}
