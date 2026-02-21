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
#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow;

class SysTray : public QObject {
    Q_OBJECT

public:
    explicit SysTray(MainWindow* mainWindow, QObject* parent = nullptr);
    ~SysTray();

    void show();
    void hide();
    void updateCurrentLayout(const QString& layoutName);
    void updateKeyboardVisible(bool visible);
    void refreshMenu();

signals:
    void layoutChanged(const QString& layoutName);
    void requestShowKeyboard();
    void requestHideKeyboard();
    void requestResetStats();
    void requestPreviewLayout();
    void requestShowAbout();
    void requestExit();

public slots:
    void onShowWindow();
    void onHideWindow();
    void onExit();

private:
    void createMenu();

    MainWindow* m_mainWindow = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_menu = nullptr;
    QString m_currentLayout;
    bool m_keyboardVisible = false;
    QMap<QString, QAction*> m_layoutActions;
    QAction* m_currentLayoutAction = nullptr;
    QAction* m_showKeyboardAction = nullptr;
};

#endif
