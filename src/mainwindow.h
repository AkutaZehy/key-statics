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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include "keyboardhook.h"
#include "mousehook.h"
#include "gamepadpoller.h"
#include "keylayout.h"
#include "virtualkeyboard.h"
#include "keystats.h"
#include "httpserver.h"
#include "layoutmanager.h"
#include "systray.h"
#include "previewwindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void setLayout(const QString& layoutFile);

public slots:
    void resetStats();
    void showAbout();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onKeyPressed(int vkCode);
    void onKeyReleased(int vkCode);
    void onMousePressed(int vkCode);
    void onMouseReleased(int vkCode);
    void onMouseMoved(int dx, int dy);
    void onGamepadAxes(int lt, int rt, int lx, int ly, int rx, int ry);
    void onLayoutLoaded(const QString& path);
    void onAutoSwitchTick();

private:
    bool loadLayout(const QString& layoutFile);
    void updateLayoutDisplayName(const QString& layoutFile);
    QString resolveLayoutPath(const QString& layoutName) const;
    QString foregroundProcessName() const;

    KeyboardHook* m_keyboardHook = nullptr;
    MouseHook* m_mouseHook = nullptr;
    QThread* m_hookThread = nullptr;
    LayoutManager* m_layoutManager = nullptr;
    QTimer* m_autoSwitchTimer = nullptr;
    bool m_autoSwitchedAway = false;
    GamepadPoller* m_gamepadPoller = nullptr;
    KeyLayout* m_layout = nullptr;
    VirtualKeyboard* m_keyboard = nullptr;
    KeyStats* m_keyStats = nullptr;
    HttpServer* m_httpServer = nullptr;
    SysTray* m_sysTray = nullptr;
    PreviewWindow* m_previewWindow = nullptr;
    QString m_currentLayoutPath;
};

#endif
