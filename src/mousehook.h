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
#ifndef MOUSEHOOK_H
#define MOUSEHOOK_H

#include <QObject>
#include <QSet>
#include <Windows.h>

class MouseHook : public QObject {
    Q_OBJECT

public:
    explicit MouseHook(QObject* parent = nullptr);
    ~MouseHook();
    
    bool start();
    void stop();
    
    const QSet<int>& pressedButtons() const { return m_pressedButtons; }

signals:
    void buttonPressed(int vkCode);
    void buttonReleased(int vkCode);
    void wheelScrolled(int delta);

private:
    static MouseHook* s_instance;
    static LRESULT CALLBACK lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    HHOOK m_hook = nullptr;
    bool m_running = false;
    QSet<int> m_pressedButtons;
};

#endif
