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
#ifndef KEYBOARDHOOK_H
#define KEYBOARDHOOK_H

#include <QObject>
#include <QSet>
#include <windows.h>

class KeyboardHook : public QObject {
    Q_OBJECT

public:
    explicit KeyboardHook(QObject* parent = nullptr);
    ~KeyboardHook();

    bool start();
    void stop();

    const QSet<int>& pressedKeys() const { return m_pressedKeys; }

signals:
    void keyPressed(int vkCode);
    void keyReleased(int vkCode);

private:
    static LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK m_hook = nullptr;
    QSet<int> m_pressedKeys;
    bool m_running = false;

    static KeyboardHook* s_instance;
};

#endif
