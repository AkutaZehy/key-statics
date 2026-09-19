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
#include <QtTest>
#include "gamepadpoller.h"

class TestGamepad : public QObject {
    Q_OBJECT

private slots:
    void buttonBitMapping();
    void stickDeadzoneAndDirection();
};

void TestGamepad::buttonBitMapping() {
    QVector<int> codes = GamepadPoller::buttonBitsToCodes(0x1000); // A only
    QCOMPARE(codes.size(), 1);
    QCOMPARE(codes.first(), GamepadVK::A);

    codes = GamepadPoller::buttonBitsToCodes(0x1001); // A + dpad up
    QCOMPARE(codes.size(), 2);
    QVERIFY(codes.contains(GamepadVK::A));
    QVERIFY(codes.contains(GamepadVK::DPadUp));

    codes = GamepadPoller::buttonBitsToCodes(0x0000);
    QVERIFY(codes.isEmpty());

    codes = GamepadPoller::buttonBitsToCodes(0xF3FF); // every mapped bit
    QCOMPARE(codes.size(), 14);
}

void TestGamepad::stickDeadzoneAndDirection() {
    // Inside the deadzone: no direction.
    QVERIFY(GamepadPoller::sticksToCodes(1000, 1000, 0, 0).isEmpty());

    // Dominant axis decides the direction; up is positive Y (XInput).
    QVector<int> codes = GamepadPoller::sticksToCodes(0, 20000, 0, 0);
    QCOMPARE(codes.size(), 1);
    QCOMPARE(codes.first(), GamepadVK::LStickUp);

    codes = GamepadPoller::sticksToCodes(20000, 0, 0, 0);
    QCOMPARE(codes.first(), GamepadVK::LStickRight);

    codes = GamepadPoller::sticksToCodes(0, 0, -20000, 5000);
    QCOMPARE(codes.first(), GamepadVK::RStickLeft);

    // Right stick above its (larger) deadzone.
    codes = GamepadPoller::sticksToCodes(0, 0, 0, -20000);
    QCOMPARE(codes.first(), GamepadVK::RStickDown);

    // Both sticks at once.
    codes = GamepadPoller::sticksToCodes(0, -20000, 20000, 0);
    QCOMPARE(codes.size(), 2);
    QVERIFY(codes.contains(GamepadVK::LStickDown));
    QVERIFY(codes.contains(GamepadVK::RStickRight));
}

QTEST_MAIN(TestGamepad)
#include "test_gamepad.moc"
