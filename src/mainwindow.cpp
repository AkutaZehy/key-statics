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
#include "mainwindow.h"
#include <QCloseEvent>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("key-statics");
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_QuitOnClose, false);
    setStyleSheet("background: transparent;");

    m_layout = new KeyLayout(this);
    m_keyboard = new VirtualKeyboard(this);
    m_keyboard->setLayout(m_layout);
    setCentralWidget(m_keyboard);

    m_keyStats = new KeyStats(this);

    QString layoutPath = QApplication::applicationDirPath() + "/layouts/104keys.json";
    if (!loadLayout(layoutPath)) {
        qWarning() << "Failed to load keyboard layout!";
    }

    m_keyboardHook = new KeyboardHook(this);
    connect(m_keyboardHook, &KeyboardHook::keyPressed, this, &MainWindow::onKeyPressed);
    connect(m_keyboardHook, &KeyboardHook::keyReleased, this, &MainWindow::onKeyReleased);

    if (!m_keyboardHook->start()) {
        qWarning() << "Failed to start keyboard hook!";
    }

    m_httpServer = new HttpServer(m_keyStats, this);
    m_httpServer->setLayout(m_layout);
    if (!m_httpServer->start(9876)) {
        qWarning() << "Failed to start HTTP server!";
    } else {
        qDebug() << "HTTP server started on port 9876";
    }

    m_sysTray = new SysTray(this, this);
}

MainWindow::~MainWindow() {
    if (m_keyboardHook) {
        m_keyboardHook->stop();
    }
    if (m_httpServer) {
        m_httpServer->stop();
    }
}

bool MainWindow::loadLayout(const QString& layoutFile) {
    if (m_layout->loadFromFile(layoutFile)) {
        if (m_keyboard) {
            m_keyboard->setLayout(m_layout);
            adjustSize();
        }
        if (m_keyStats) {
            QSet<int> validKeys;
            for (int vk : m_layout->keys().keys()) {
                validKeys.insert(vk);
            }
            m_keyStats->setValidKeys(validKeys);
        }
        return true;
    }
    return false;
}

void MainWindow::setLayout(const QString& layoutFile) {
    loadLayout(layoutFile);
}

void MainWindow::onKeyPressed(int vkCode) {
    if (m_keyboard) {
        m_keyboard->onKeyPressed(vkCode);
    }
    if (m_keyStats) {
        m_keyStats->recordKeyPress(vkCode);
    }
}

void MainWindow::onKeyReleased(int vkCode) {
    if (m_keyboard) {
        m_keyboard->onKeyReleased(vkCode);
    }
    if (m_keyStats) {
        m_keyStats->recordKeyRelease(vkCode);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
}
