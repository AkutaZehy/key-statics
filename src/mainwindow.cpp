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

    // Layouts: directory scan + watcher-driven hot reload. All loads (initial,
    // tray, auto-switch) go through the manager; side effects apply in
    // onLayoutLoaded.
    m_layoutManager = new LayoutManager(this);
    m_layoutManager->bindLayout(m_layout);
    m_layoutManager->setLayoutDir(QApplication::applicationDirPath() + "/layouts");
    connect(m_layoutManager, &LayoutManager::layoutLoaded, this, &MainWindow::onLayoutLoaded);
    connect(m_layoutManager, &LayoutManager::layoutListChanged, this, [this]() {
        if (m_sysTray) {
            m_sysTray->setLayoutFiles(m_layoutManager->availableLayouts());
        }
    });
    connect(m_layoutManager, &LayoutManager::layoutLoadFailed, this, [](const QString& path) {
        qWarning() << "Layout load failed, keeping previous layout:" << path;
    });

    const QString defaultPath = resolveLayoutPath(Config::instance()->defaultLayout());
    if (!m_layoutManager->loadLayout(defaultPath)) {
        qWarning() << "Failed to load default layout:" << defaultPath;
    }

    // Low-level hooks live on a dedicated thread: if the GUI thread ever
    // stalls, Windows would otherwise silently drop the system-wide hooks
    // after a timeout and stats would stop without any error.
    m_hookThread = new QThread(this);
    m_hookThread->setObjectName("input-hook");

    m_keyboardHook = new KeyboardHook();
    m_keyboardHook->moveToThread(m_hookThread);
    connect(m_hookThread, &QThread::started, m_keyboardHook, &KeyboardHook::start);
    connect(m_hookThread, &QThread::finished, m_keyboardHook, &QObject::deleteLater);

    m_mouseHook = new MouseHook();
    m_mouseHook->moveToThread(m_hookThread);
    connect(m_hookThread, &QThread::started, m_mouseHook, &MouseHook::start);
    connect(m_hookThread, &QThread::finished, m_mouseHook, &QObject::deleteLater);

    connect(m_keyboardHook, &KeyboardHook::keyPressed, this, &MainWindow::onKeyPressed);
    connect(m_keyboardHook, &KeyboardHook::keyReleased, this, &MainWindow::onKeyReleased);
    connect(m_mouseHook, &MouseHook::buttonPressed, this, &MainWindow::onMousePressed);
    connect(m_mouseHook, &MouseHook::buttonReleased, this, &MainWindow::onMouseReleased);
    connect(m_mouseHook, &MouseHook::mouseMoved, this, &MainWindow::onMouseMoved);

    if (Config::instance()->gamepadEnabled()) {
        m_gamepadPoller = new GamepadPoller();
        m_gamepadPoller->moveToThread(m_hookThread);
        connect(m_hookThread, &QThread::started, m_gamepadPoller, [this]() {
            m_gamepadPoller->start(Config::instance()->gamepadUserIndex());
        });
        connect(m_gamepadPoller, &GamepadPoller::buttonPressed, this, &MainWindow::onMousePressed);
        connect(m_gamepadPoller, &GamepadPoller::buttonReleased, this, &MainWindow::onMouseReleased);
        connect(m_gamepadPoller, &GamepadPoller::axesChanged, this, &MainWindow::onGamepadAxes);
    }

    m_hookThread->start();

    m_httpServer = new HttpServer(m_keyStats, this);
    m_httpServer->setLayout(m_layout);
    
    quint16 port = Config::instance()->serverPort();
    if (!m_httpServer->start(port)) {
        qWarning() << "Failed to start HTTP server!";
    } else {
        qDebug() << "HTTP server started on port" << port;
    }

    m_sysTray = new SysTray(this, this);
    m_sysTray->setLayoutFiles(m_layoutManager->availableLayouts());
    m_sysTray->updateCurrentLayout(QFileInfo(m_currentLayoutPath).fileName());

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
            connect(m_keyboardHook, &KeyboardHook::keyPressed, m_previewWindow, &PreviewWindow::onKeyPressed);
            connect(m_keyboardHook, &KeyboardHook::keyReleased, m_previewWindow, &PreviewWindow::onKeyReleased);
            connect(m_mouseHook, &MouseHook::buttonPressed, m_previewWindow, &PreviewWindow::onMousePressed);
            connect(m_mouseHook, &MouseHook::buttonReleased, m_previewWindow, &PreviewWindow::onMouseReleased);
            if (m_gamepadPoller) {
                connect(m_gamepadPoller, &GamepadPoller::buttonPressed, m_previewWindow, &PreviewWindow::onMousePressed);
                connect(m_gamepadPoller, &GamepadPoller::buttonReleased, m_previewWindow, &PreviewWindow::onMouseReleased);
                connect(m_gamepadPoller, &GamepadPoller::axesChanged, m_previewWindow, &PreviewWindow::onGamepadAxes);
            }
            connect(m_layoutManager, &LayoutManager::layoutListChanged, m_previewWindow, &PreviewWindow::refreshLayouts);
        }
        m_previewWindow->show();
    });
    connect(m_sysTray, &SysTray::layoutChanged, this, &MainWindow::updateLayoutDisplayName);

    updateLayoutDisplayName(m_currentLayoutPath);
}

MainWindow::~MainWindow() {
    if (m_hookThread) {
        if (m_keyboardHook) {
            QMetaObject::invokeMethod(m_keyboardHook, "stop", Qt::BlockingQueuedConnection);
        }
        if (m_mouseHook) {
            QMetaObject::invokeMethod(m_mouseHook, "stop", Qt::BlockingQueuedConnection);
        }
        if (m_gamepadPoller) {
            QMetaObject::invokeMethod(m_gamepadPoller, "stop", Qt::BlockingQueuedConnection);
        }
        m_hookThread->quit();
        m_hookThread->wait();
    }
    if (m_httpServer) {
        m_httpServer->stop();
    }
}

bool MainWindow::loadLayout(const QString& layoutFile) {
    // Side effects only: LayoutManager already parsed the file into
    // m_layout (and only on a fully successful parse).
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

    m_currentLayoutPath = layoutFile;
    return true;
}

void MainWindow::onLayoutLoaded(const QString& path) {
    loadLayout(path);

    if (m_sysTray) {
        m_sysTray->updateCurrentLayout(QFileInfo(path).fileName());
        m_sysTray->refreshMenu();
    }
    // Browser sources refetch the page when the layout changed on disk.
    if (m_httpServer) {
        m_httpServer->notifyLayoutChanged();
    }
}

void MainWindow::setLayout(const QString& layoutFile) {
    m_layoutManager->loadLayout(layoutFile);
}

QString MainWindow::resolveLayoutPath(const QString& layoutName) const {
    QString name = layoutName;
    if (name.endsWith(".json", Qt::CaseInsensitive)) {
        name.chop(5);
    }
    const QString dir = QApplication::applicationDirPath() + "/layouts/";
    const QString path = dir + name + ".json";
    if (QFileInfo::exists(path)) {
        return path;
    }
    return dir + "104keys.json";
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

void MainWindow::onMouseMoved(int dx, int dy) {
    if (m_keyStats) {
        m_keyStats->recordMouseMotion(dx, dy);
    }
    if (m_keyboard) {
        m_keyboard->onMouseMotion(dx, dy);
    }
    if (m_previewWindow) {
        m_previewWindow->onMouseMotion(dx, dy);
    }
}

void MainWindow::onGamepadAxes(int lt, int rt, int lx, int ly, int rx, int ry) {
    if (m_keyStats) {
        m_keyStats->recordGamepadAxes(lt, rt, lx, ly, rx, ry);
    }
    if (m_keyboard) {
        m_keyboard->onGamepadAxes(lt, rt, lx, ly, rx, ry);
    }
    if (m_previewWindow) {
        m_previewWindow->onGamepadAxes(lt, rt, lx, ly, rx, ry);
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
    
    QLabel* version = new QLabel(QString("Version: v%1").arg(KEY_STATICS_VERSION), &aboutDialog);
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
