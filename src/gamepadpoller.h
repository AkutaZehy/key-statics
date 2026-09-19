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
#ifndef GAMEPADPOLLER_H
#define GAMEPADPOLLER_H

#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <windows.h>

using XInputGetStateFn = DWORD(WINAPI*)(DWORD dwUserIndex, void* pState);

// Gamepad virtual-key segment: disjoint from mouse buttons (0x01-0x06),
// keyboard VKs (<= 0xFF) and layout elements (0x200+).
namespace GamepadVK {
    constexpr int DPadUp      = 0x110;
    constexpr int DPadDown    = 0x111;
    constexpr int DPadLeft    = 0x112;
    constexpr int DPadRight   = 0x113;
    constexpr int Start       = 0x114;
    constexpr int Back        = 0x115;
    constexpr int LStickPress = 0x116;
    constexpr int RStickPress = 0x117;
    constexpr int LBumper     = 0x118;
    constexpr int RBumper     = 0x119;
    constexpr int A           = 0x11A;
    constexpr int B           = 0x11B;
    constexpr int X           = 0x11C;
    constexpr int Y           = 0x11D;
    constexpr int LTrigger    = 0x11E;
    constexpr int RTrigger    = 0x11F;
    constexpr int LStickUp    = 0x120;
    constexpr int LStickDown  = 0x121;
    constexpr int LStickLeft  = 0x122;
    constexpr int LStickRight = 0x123;
    constexpr int RStickUp    = 0x124;
    constexpr int RStickDown  = 0x125;
    constexpr int RStickLeft  = 0x126;
    constexpr int RStickRight = 0x127;
}

// Polls an XInput controller and reports press/release edges as virtual
// key codes, plus the raw analog axes for gauge rendering. There is no
// global-hook API for gamepads, so polling is the mechanism; with no
// controller connected the timer keeps running but does nothing.
class GamepadPoller : public QObject {
    Q_OBJECT

public:
    explicit GamepadPoller(QObject* parent = nullptr);
    ~GamepadPoller() override;

    // Mapping helpers exposed for unit tests.
    static QVector<int> buttonBitsToCodes(unsigned short wButtons);
    static QVector<int> sticksToCodes(short lx, short ly, short rx, short ry);

public slots:
    // Runs on the worker thread. XInputGetState is resolved dynamically
    // (xinput1_4 / 1_3 / 9_1_0) so the build needs no xinput import lib.
    bool start(int userIndex);
    void stop();

signals:
    void buttonPressed(int vkCode);
    void buttonReleased(int vkCode);
    // Raw axis values (lt/rt: 0-255, sticks: -32768..32767), emitted only
    // when something changed while the controller is connected.
    void axesChanged(int lt, int rt, int lx, int ly, int rx, int ry);

private slots:
    void poll();

private:
    QTimer* m_timer = nullptr;
    int m_userIndex = 0;
    bool m_connected = false;
    bool m_started = false;
    QSet<int> m_pressed;
    XInputGetStateFn m_getState = nullptr;
    int m_lastLt = -1;
    int m_lastRt = -1;
    int m_lastLx = 1;  // sentinel != any real quantized value
    int m_lastLy = 1;
    int m_lastRx = 1;
    int m_lastRy = 1;
};

#endif
