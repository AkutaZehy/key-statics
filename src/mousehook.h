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
#include <QTimer>
#include <atomic>
#include <Windows.h>
#include "inputconsts.h"

class MouseHook : public QObject {
    Q_OBJECT

public:
    explicit MouseHook(QObject* parent = nullptr);
    ~MouseHook();

public slots:
    // Invoked on the worker thread that owns this object, so the low-level
    // hook is installed and serviced there instead of the GUI thread.
    bool start();
    void stop();

signals:
    void buttonPressed(int vkCode);
    void buttonReleased(int vkCode);

    // Emitted at MOUSE_MOTION_SAMPLE_MS with the raw accumulated delta
    // since the previous sample; receivers convert to px/s themselves.
    void mouseMoved(int dx, int dy);

private slots:
    void sampleMotion();

private:
    static MouseHook* s_instance;
    static LRESULT CALLBACK lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    void accumulateMouseMotion(const POINT& pt);

    HHOOK m_hook = nullptr;
    bool m_running = false;
    QSet<int> m_pressedButtons;

    // Motion accumulator. The hook proc and sampleMotion() both run on the
    // hook thread; atomics only as cheap insurance against thread moves.
    QTimer* m_motionTimer = nullptr;
    std::atomic<int> m_pendingDx{0};
    std::atomic<int> m_pendingDy{0};
    int m_lastX = 0;
    int m_lastY = 0;
    bool m_hasLastPt = false;
};

#endif
