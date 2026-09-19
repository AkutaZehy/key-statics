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
#include "gamepadpoller.h"
#include <QLibrary>
#include <QPair>
#include <utility>
#include <QDebug>
#include <cmath>

// Layout-compatible mirror of XINPUT_STATE (avoids including xinput.h).
struct XInputState {
    unsigned long packetNumber;
    unsigned short wButtons;
    unsigned char leftTrigger;
    unsigned char rightTrigger;
    short sThumbLX;
    short sThumbLY;
    short sThumbRX;
    short sThumbRY;
};

static_assert(sizeof(XInputState) == 16, "XInputState layout mismatch");

namespace {
constexpr unsigned long kErrorSuccess = 0;
constexpr unsigned short kDpadUp = 0x0001;
constexpr unsigned short kDpadDown = 0x0002;
constexpr unsigned short kDpadLeft = 0x0004;
constexpr unsigned short kDpadRight = 0x0008;
constexpr unsigned short kStart = 0x0010;
constexpr unsigned short kBack = 0x0020;
constexpr unsigned short kLeftThumb = 0x0040;
constexpr unsigned short kRightThumb = 0x0080;
constexpr unsigned short kLeftShoulder = 0x0100;
constexpr unsigned short kRightShoulder = 0x0200;
constexpr unsigned short kButtonA = 0x1000;
constexpr unsigned short kButtonB = 0x2000;
constexpr unsigned short kButtonX = 0x4000;
constexpr unsigned short kButtonY = 0x8000;

constexpr short kLeftThumbDeadzone = 7849;
constexpr short kRightThumbDeadzone = 8689;
constexpr short kTriggerThreshold = 30;
} // namespace

GamepadPoller::GamepadPoller(QObject* parent)
    : QObject(parent)
{
}

GamepadPoller::~GamepadPoller() {
    stop();
}

QVector<int> GamepadPoller::buttonBitsToCodes(unsigned short wButtons) {
    QVector<int> codes;
    const QPair<unsigned short, int> map[] = {
        {kDpadUp, GamepadVK::DPadUp},       {kDpadDown, GamepadVK::DPadDown},
        {kDpadLeft, GamepadVK::DPadLeft},   {kDpadRight, GamepadVK::DPadRight},
        {kStart, GamepadVK::Start},         {kBack, GamepadVK::Back},
        {kLeftThumb, GamepadVK::LStickPress}, {kRightThumb, GamepadVK::RStickPress},
        {kLeftShoulder, GamepadVK::LBumper}, {kRightShoulder, GamepadVK::RBumper},
        {kButtonA, GamepadVK::A},           {kButtonB, GamepadVK::B},
        {kButtonX, GamepadVK::X},           {kButtonY, GamepadVK::Y},
    };
    for (const auto& entry : map) {
        if (wButtons & entry.first) {
            codes.append(entry.second);
        }
    }
    return codes;
}

QVector<int> GamepadPoller::sticksToCodes(short lx, short ly, short rx, short ry) {
    QVector<int> codes;

    auto stickCode = [](short x, short y, short deadzone, int up, int down, int left, int right) -> int {
        const double magnitude = std::hypot(static_cast<double>(x), static_cast<double>(y));
        if (magnitude < deadzone) {
            return 0;
        }
        if (std::abs(x) > std::abs(y)) {
            return x > 0 ? right : left;
        }
        return y > 0 ? up : down;
    };

    const int left = stickCode(lx, ly, kLeftThumbDeadzone,
                               GamepadVK::LStickUp, GamepadVK::LStickDown,
                               GamepadVK::LStickLeft, GamepadVK::LStickRight);
    if (left) codes.append(left);

    const int right = stickCode(rx, ry, kRightThumbDeadzone,
                                GamepadVK::RStickUp, GamepadVK::RStickDown,
                                GamepadVK::RStickLeft, GamepadVK::RStickRight);
    if (right) codes.append(right);

    return codes;
}

bool GamepadPoller::start(int userIndex) {
    if (m_started) {
        return true;
    }

    for (const QString& dll : {QStringLiteral("xinput1_4"), QStringLiteral("xinput1_3"), QStringLiteral("xinput9_1_0")}) {
        QLibrary lib(dll);
        if (lib.load()) {
            m_getState = reinterpret_cast<XInputGetStateFn>(lib.resolve("XInputGetState"));
            if (m_getState) {
                break;
            }
        }
    }
    if (!m_getState) {
        qWarning() << "Gamepad support unavailable: XInputGetState not found";
        return false;
    }

    m_userIndex = userIndex;
    m_connected = false;
    m_pressed.clear();
    m_lastLt = m_lastRt = -1;
    m_lastLx = m_lastLy = m_lastRx = m_lastRy = 1;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GamepadPoller::poll);
    m_timer->start(16);

    m_started = true;
    qDebug() << "Gamepad poller started for user index" << m_userIndex;
    return true;
}

void GamepadPoller::stop() {
    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    m_started = false;
    m_connected = false;
    m_pressed.clear();
}

void GamepadPoller::poll() {
    if (!m_getState) {
        return;
    }

    XInputState state{};
    if (m_getState(static_cast<unsigned long>(m_userIndex), &state) != kErrorSuccess) {
        if (m_connected) {
            // Controller detached: release everything so no key sticks on.
            for (int vk : std::as_const(m_pressed)) {
                emit buttonReleased(vk);
            }
            m_pressed.clear();
            m_connected = false;
        }
        return;
    }
    m_connected = true;

    QSet<int> now;
    for (int code : buttonBitsToCodes(state.wButtons)) {
        now.insert(code);
    }
    for (int code : sticksToCodes(state.sThumbLX, state.sThumbLY, state.sThumbRX, state.sThumbRY)) {
        now.insert(code);
    }
    if (state.leftTrigger >= kTriggerThreshold) {
        now.insert(GamepadVK::LTrigger);
    }
    if (state.rightTrigger >= kTriggerThreshold) {
        now.insert(GamepadVK::RTrigger);
    }

    for (int vk : std::as_const(now)) {
        if (!m_pressed.contains(vk)) {
            m_pressed.insert(vk);
            emit buttonPressed(vk);
        }
    }
    for (int vk : std::as_const(m_pressed)) {
        if (!now.contains(vk)) {
            m_pressed.remove(vk);
            emit buttonReleased(vk);
        }
    }

    const int lt = state.leftTrigger;
    const int rt = state.rightTrigger;
    const int lx = state.sThumbLX;
    const int ly = state.sThumbLY;
    const int rx = state.sThumbRX;
    const int ry = state.sThumbRY;
    if (lt != m_lastLt || rt != m_lastRt || lx != m_lastLx || ly != m_lastLy || rx != m_lastRx || ry != m_lastRy) {
        m_lastLt = lt;
        m_lastRt = rt;
        m_lastLx = lx;
        m_lastLy = ly;
        m_lastRx = rx;
        m_lastRy = ry;
        emit axesChanged(lt, rt, lx, ly, rx, ry);
    }
}
