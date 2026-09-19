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

    m_motionTimer = new QTimer(this);
    connect(m_motionTimer, &QTimer::timeout, this, &MouseHook::sampleMotion);
    m_motionTimer->start(MOUSE_MOTION_SAMPLE_MS);

    m_running = true;
    qDebug() << "Mouse hook started";
    return true;
}

void MouseHook::stop() {
    if (m_motionTimer) {
        m_motionTimer->stop();
        m_motionTimer->deleteLater();
        m_motionTimer = nullptr;
    }
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    m_running = false;
    m_pressedButtons.clear();
    m_pendingDx.store(0, std::memory_order_relaxed);
    m_pendingDy.store(0, std::memory_order_relaxed);
    m_hasLastPt = false;
    qDebug() << "Mouse hook stopped";
}

void MouseHook::accumulateMouseMotion(const POINT& pt) {
    if (m_hasLastPt) {
        int dx = pt.x - m_lastX;
        int dy = pt.y - m_lastY;
        // Clamp cursor teleports (remote desktop, games resetting the cursor)
        // so a single jump cannot spike the whole sample period.
        dx = qBound(-1000, dx, 1000);
        dy = qBound(-1000, dy, 1000);
        if (dx || dy) {
            m_pendingDx.fetch_add(dx, std::memory_order_relaxed);
            m_pendingDy.fetch_add(dy, std::memory_order_relaxed);
        }
    }
    m_lastX = pt.x;
    m_lastY = pt.y;
    m_hasLastPt = true;
}

void MouseHook::sampleMotion() {
    int dx = m_pendingDx.exchange(0, std::memory_order_relaxed);
    int dy = m_pendingDy.exchange(0, std::memory_order_relaxed);
    if (dx || dy) {
        emit mouseMoved(dx, dy);
    }
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
            case WM_MOUSEMOVE:
                s_instance->accumulateMouseMotion(pMouse->pt);
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
