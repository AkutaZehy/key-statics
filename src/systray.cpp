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
#include <QMessageBox>

#include "config.h"

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
    m_layoutActions.clear();
    
    QMenu* layoutMenu = new QMenu("Switch Layout", m_menu);
    
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
            connect(action, &QAction::triggered, this, [this, fullPath, fileName]() {
                m_mainWindow->setLayout(fullPath);
                updateCurrentLayout(fileName);
                emit layoutChanged(fullPath);
            });
            m_layoutActions[fileName] = action;
            layoutMenu->addAction(action);
        }
    }
    
    m_menu->addMenu(layoutMenu);
    m_menu->addSeparator();
    
    m_currentLayoutAction = new QAction("Current: --", this);
    m_currentLayoutAction->setEnabled(false);
    m_menu->addAction(m_currentLayoutAction);
    
    m_menu->addSeparator();
    
    QAction* resetAction = new QAction("Reset Stats", this);
    connect(resetAction, &QAction::triggered, this, &SysTray::requestResetStats);
    m_menu->addAction(resetAction);
    
    m_menu->addSeparator();
    
    m_showKeyboardAction = new QAction("Show Keyboard", this);
    connect(m_showKeyboardAction, &QAction::triggered, this, &SysTray::requestShowKeyboard);
    m_menu->addAction(m_showKeyboardAction);
    
    QAction* previewAction = new QAction("Preview Layout...", this);
    connect(previewAction, &QAction::triggered, this, &SysTray::requestPreviewLayout);
    m_menu->addAction(previewAction);
    
    m_menu->addSeparator();
    
    QAction* aboutAction = new QAction("About", this);
    connect(aboutAction, &QAction::triggered, this, &SysTray::requestShowAbout);
    m_menu->addAction(aboutAction);
    
    QAction* exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &SysTray::onExit);
    m_menu->addAction(exitAction);
}

void SysTray::refreshMenu() {
    for (auto it = m_layoutActions.constBegin(); it != m_layoutActions.constEnd(); ++it) {
        QString fileName = it.key();
        QAction* action = it.value();
        if (fileName == m_currentLayout) {
            action->setText("* " + fileName);
        } else {
            action->setText(fileName);
        }
    }
    
    if (m_currentLayoutAction) {
        m_currentLayoutAction->setText("Current: " + (m_currentLayout.isEmpty() ? "--" : m_currentLayout));
    }
    
    if (m_showKeyboardAction) {
        m_showKeyboardAction->setText(m_keyboardVisible ? "Hide Keyboard" : "Show Keyboard");
    }
    
    m_trayIcon->setToolTip(QString("key-statics%1").arg(m_currentLayout.isEmpty() ? "" : QString(" - %1").arg(m_currentLayout)));
}

void SysTray::updateCurrentLayout(const QString& layoutName) {
    m_currentLayout = layoutName;
    refreshMenu();
}

void SysTray::updateKeyboardVisible(bool visible) {
    m_keyboardVisible = visible;
    refreshMenu();
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
