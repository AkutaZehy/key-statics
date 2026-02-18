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
#include "systray.h"
#include "mainwindow.h"
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QDebug>
#include <QIcon>
#include <QDir>

SysTray::SysTray(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip("key-statics");
    
    QIcon icon(QApplication::applicationDirPath() + "/key-statics.png");
    m_trayIcon->setIcon(icon);

    createMenu();

    m_trayIcon->setContextMenu(m_menu);

    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (m_mainWindow->isVisible()) {
                m_mainWindow->hide();
            } else {
                m_mainWindow->show();
            }
        }
    });
}

SysTray::~SysTray() {
}

void SysTray::createMenu() {
    m_menu = new QMenu();

    QString layoutDir = QApplication::applicationDirPath() + "/layouts";
    QDir dir(layoutDir);
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files | QDir::Readable);

    if (jsonFiles.isEmpty()) {
        QString defaultFile = layoutDir + "/104keys.json";
        QFile file(defaultFile);
        if (!file.exists()) {
            qWarning() << "No layout files found!";
        }
    } else {
        for (const QString& fileName : jsonFiles) {
            QString fullPath = layoutDir + "/" + fileName;
            QAction* action = new QAction(fileName, this);
            connect(action, &QAction::triggered, this, [this, fullPath]() {
                m_mainWindow->setLayout(fullPath);
            });
            m_menu->addAction(action);
        }
    }

    m_menu->addSeparator();

    QAction* exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, this, &SysTray::onExit);
    m_menu->addAction(exitAction);
}

void SysTray::show() {
    if (m_trayIcon) {
        m_trayIcon->show();
    }
}

void SysTray::hide() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

void SysTray::onShowWindow() {
    if (m_mainWindow) {
        if (m_mainWindow->isVisible()) {
            m_mainWindow->hide();
        } else {
            m_mainWindow->show();
        }
    }
}

void SysTray::onHideWindow() {
    if (m_mainWindow) {
        m_mainWindow->hide();
    }
}

void SysTray::onExit() {
    QApplication::quit();
}
