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
#include "mousehook.h"
#include <QDebug>

MouseHook* MouseHook::s_instance = nullptr;

MouseHook::MouseHook(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
}

MouseHook::~MouseHook() {
    stop();
    s_instance = nullptr;
}

bool MouseHook::start() {
    if (m_running) {
        return true;
    }
    
    m_hook = SetWindowsHookEx(WH_MOUSE_LL, lowLevelMouseProc, GetModuleHandle(nullptr), 0);
    if (!m_hook) {
        qWarning() << "Failed to install mouse hook:" << GetLastError();
        return false;
    }
    
    m_running = true;
    qDebug() << "Mouse hook started";
    return true;
}

void MouseHook::stop() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    m_running = false;
    m_pressedButtons.clear();
    qDebug() << "Mouse hook stopped";
}

LRESULT CALLBACK MouseHook::lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (s_instance) {
            MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            
            int vkCode = 0;
            switch (wParam) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                vkCode = 0x01;
                break;
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                vkCode = 0x02;
                break;
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                vkCode = 0x04;
                break;
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                vkCode = HIWORD(pMouse->mouseData) == XBUTTON1 ? 0x05 : 0x06;
                break;
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                {
                    int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                    if (delta > 0) {
                        emit s_instance->wheelScrolled(1);
                    } else if (delta < 0) {
                        emit s_instance->wheelScrolled(-1);
                    }
                }
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            default:
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            
            switch (wParam) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                if (!s_instance->m_pressedButtons.contains(vkCode)) {
                    s_instance->m_pressedButtons.insert(vkCode);
                    emit s_instance->buttonPressed(vkCode);
                }
                break;
                
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
                if (s_instance->m_pressedButtons.contains(vkCode)) {
                    s_instance->m_pressedButtons.remove(vkCode);
                    emit s_instance->buttonReleased(vkCode);
                }
                break;
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
