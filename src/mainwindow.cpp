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
#include <QFileInfo>

#include "config.h"

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

    QString defaultLayout = Config::instance()->defaultLayout();
    QString layoutPath = QApplication::applicationDirPath() + "/layouts/" + defaultLayout + ".json";
    
    if (!QFileInfo::exists(layoutPath)) {
        layoutPath = QApplication::applicationDirPath() + "/layouts/104keys.json";
    }
    
    if (!loadLayout(layoutPath)) {
        qWarning() << "Failed to load keyboard layout!";
    }

    m_keyboardHook = new KeyboardHook(this);
    connect(m_keyboardHook, &KeyboardHook::keyPressed, this, &MainWindow::onKeyPressed);
    connect(m_keyboardHook, &KeyboardHook::keyReleased, this, &MainWindow::onKeyReleased);

    if (!m_keyboardHook->start()) {
        qWarning() << "Failed to start keyboard hook!";
    }
    
    m_mouseHook = new MouseHook(this);
    connect(m_mouseHook, &MouseHook::buttonPressed, this, &MainWindow::onMousePressed);
    connect(m_mouseHook, &MouseHook::buttonReleased, this, &MainWindow::onMouseReleased);
    
    if (!m_mouseHook->start()) {
        qWarning() << "Failed to start mouse hook!";
    }

    m_httpServer = new HttpServer(m_keyStats, this);
    m_httpServer->setLayout(m_layout);
    
    quint16 port = Config::instance()->serverPort();
    if (!m_httpServer->start(port)) {
        qWarning() << "Failed to start HTTP server!";
    } else {
        qDebug() << "HTTP server started on port" << port;
    }

    m_sysTray = new SysTray(this, this);
    
    connect(m_sysTray, &SysTray::requestResetStats, this, &MainWindow::resetStats);
    connect(m_sysTray, &SysTray::requestShowAbout, this, &MainWindow::showAbout);
    connect(m_sysTray, &SysTray::requestShowKeyboard, this, [this]() { 
        if (isVisible()) {
            hide();
            m_sysTray->updateKeyboardVisible(false);
        } else {
            show();
            m_sysTray->updateKeyboardVisible(true);
        } 
    });
    connect(m_sysTray, &SysTray::requestPreviewLayout, this, [this]() {
        if (!m_previewWindow) {
            m_previewWindow = new PreviewWindow();
        }
        m_previewWindow->show();
    });
    connect(m_sysTray, &SysTray::layoutChanged, this, &MainWindow::updateLayoutDisplayName);
    
    updateLayoutDisplayName(layoutPath);
}

MainWindow::~MainWindow() {
    if (m_keyboardHook) {
        m_keyboardHook->stop();
    }
    if (m_mouseHook) {
        m_mouseHook->stop();
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
        
        if (m_httpServer) {
            m_httpServer->setLayout(m_layout);
        }
        
        return true;
    }
    return false;
}

void MainWindow::setLayout(const QString& layoutFile) {
    m_currentLayoutPath = layoutFile;
    loadLayout(layoutFile);
    updateLayoutDisplayName(layoutFile);
}

void MainWindow::updateLayoutDisplayName(const QString& layoutFile) {
    QFileInfo fileInfo(layoutFile);
    QString layoutName = fileInfo.fileName();
    m_currentLayoutPath = layoutFile;
    
    if (m_sysTray) {
        m_sysTray->updateCurrentLayout(layoutName);
    }
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

void MainWindow::onMousePressed(int vkCode) {
    if (m_keyboard) {
        m_keyboard->onKeyPressed(vkCode);
    }
    if (m_keyStats) {
        m_keyStats->recordKeyPress(vkCode);
    }
}

void MainWindow::onMouseReleased(int vkCode) {
    if (m_keyboard) {
        m_keyboard->onKeyReleased(vkCode);
    }
    if (m_keyStats) {
        m_keyStats->recordKeyRelease(vkCode);
    }
}

void MainWindow::resetStats() {
    if (m_keyStats) {
        m_keyStats->reset();
    }
}

void MainWindow::showAbout() {
    QDialog aboutDialog(nullptr);
    aboutDialog.setWindowTitle("About");
    aboutDialog.setMinimumSize(350, 200);
    
    QVBoxLayout* layout = new QVBoxLayout(&aboutDialog);
    
    QLabel* title = new QLabel("key-statics", &aboutDialog);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    
    QLabel* desc1 = new QLabel("Lightweight keyboard input display", &aboutDialog);
    desc1->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc1);
    
    QLabel* desc2 = new QLabel("For OBS live streaming", &aboutDialog);
    desc2->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc2);
    
    layout->addSpacing(15);
    
    QLabel* version = new QLabel(QString("Version: v%1").arg("1.2.0"), &aboutDialog);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);
    
    QLabel* tech = new QLabel("Qt 6.10.2 | GPL v3", &aboutDialog);
    tech->setAlignment(Qt::AlignCenter);
    layout->addWidget(tech);
    
    layout->addSpacing(10);
    
    QLabel* github = new QLabel("<a href=\"https://github.com/AkutaZehy/key-statics\">https://github.com/AkutaZehy/key-statics</a>", &aboutDialog);
    github->setOpenExternalLinks(true);
    github->setAlignment(Qt::AlignCenter);
    layout->addWidget(github);
    
    layout->addStretch();
    
    QPushButton* okBtn = new QPushButton("OK", &aboutDialog);
    okBtn->setFixedWidth(80);
    connect(okBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
    layout->addWidget(okBtn, 0, Qt::AlignCenter);
    
    aboutDialog.exec();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
}
