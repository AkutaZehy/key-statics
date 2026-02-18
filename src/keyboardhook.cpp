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
#include "keyboardhook.h"
#include <QDebug>

KeyboardHook* KeyboardHook::s_instance = nullptr;

KeyboardHook::KeyboardHook(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

KeyboardHook::~KeyboardHook() {
    stop();
    s_instance = nullptr;
}

bool KeyboardHook::start() {
    if (m_running) {
        return true;
    }

    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, lowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    if (!m_hook) {
        qWarning() << "Failed to install keyboard hook:" << GetLastError();
        return false;
    }

    m_running = true;
    qDebug() << "Keyboard hook started";
    return true;
}

void KeyboardHook::stop() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    m_running = false;
    qDebug() << "Keyboard hook stopped";
}

LRESULT CALLBACK KeyboardHook::lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (s_instance) {
            KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            int vkCode = static_cast<int>(pKeyboard->vkCode);

            switch (wParam) {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (!s_instance->m_pressedKeys.contains(vkCode)) {
                    s_instance->m_pressedKeys.insert(vkCode);
                    emit s_instance->keyPressed(vkCode);
                }
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (s_instance->m_pressedKeys.contains(vkCode)) {
                    s_instance->m_pressedKeys.remove(vkCode);
                    emit s_instance->keyReleased(vkCode);
                }
                break;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
