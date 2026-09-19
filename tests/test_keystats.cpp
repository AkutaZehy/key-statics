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
#include "keystats.h"

class TestKeyStats : public QObject {
    Q_OBJECT

private slots:
    void pressIncrementsCounters();
    void releaseRemovesPressedState();
    void releaseUnknownKeyEmitsNothing();
    void validKeysFilter();
    void resetClearsEverything();
    void kpsStaysSilentWhenIdle();
    void mouseMotionSmoothingAndDecay();
};

void TestKeyStats::pressIncrementsCounters() {
    KeyStats stats;
    QSignalSpy spy(&stats, &KeyStats::statsUpdated);

    stats.recordKeyPress(65);
    QCOMPARE(stats.totalKeyPresses(), 1);
    QCOMPARE(stats.keyCounts().value(65), 1);
    QVERIFY(stats.pressedKeys().contains(65));
    QCOMPARE(spy.count(), 1);

    stats.recordKeyPress(65);
    QCOMPARE(stats.totalKeyPresses(), 2);
    QCOMPARE(stats.keyCounts().value(65), 2);
    QCOMPARE(spy.count(), 2);
}

void TestKeyStats::releaseRemovesPressedState() {
    KeyStats stats;
    QSignalSpy spy(&stats, &KeyStats::statsUpdated);

    stats.recordKeyPress(66);
    spy.clear();
    stats.recordKeyRelease(66);

    QVERIFY(!stats.pressedKeys().contains(66));
    QCOMPARE(spy.count(), 1);
}

void TestKeyStats::releaseUnknownKeyEmitsNothing() {
    KeyStats stats;
    QSignalSpy spy(&stats, &KeyStats::statsUpdated);

    stats.recordKeyRelease(65);

    QCOMPARE(spy.count(), 0);
}

void TestKeyStats::validKeysFilter() {
    KeyStats stats;
    stats.setValidKeys(QSet<int>{68, 70});
    QSignalSpy spy(&stats, &KeyStats::statsUpdated);

    stats.recordKeyPress(65);
    QCOMPARE(stats.totalKeyPresses(), 0);
    QVERIFY(!stats.pressedKeys().contains(65));
    QCOMPARE(spy.count(), 0);

    stats.recordKeyPress(68);
    QCOMPARE(stats.totalKeyPresses(), 1);
    QCOMPARE(stats.keyCounts().value(68), 1);
}

void TestKeyStats::resetClearsEverything() {
    KeyStats stats;
    stats.recordKeyPress(65);
    stats.recordKeyRelease(65);

    stats.reset();

    QCOMPARE(stats.totalKeyPresses(), 0);
    QCOMPARE(stats.kps(), 0);
    QVERIFY(stats.keyCounts().isEmpty());
    QVERIFY(stats.pressedKeys().isEmpty());
}

void TestKeyStats::kpsStaysSilentWhenIdle() {
    KeyStats stats;
    QSignalSpy spy(&stats, &KeyStats::statsUpdated);

    // The KPS timer fires every 100 ms; with no input the smoothed KPS stays
    // at 0 and statsUpdated must not be emitted, so idle SSE clients get no frames.
    QTest::qWait(350);

    QCOMPARE(spy.count(), 0);
    QCOMPARE(stats.kps(), 0);
}

void TestKeyStats::mouseMotionSmoothingAndDecay() {
    KeyStats stats;

    stats.recordMouseMotion(100, 0);
    QVERIFY(stats.mouseVelocityX() > 0);
    QCOMPARE(stats.mouseVelocityY(), 0);

    // Motion events keep the smoothed velocity alive...
    QTest::qWait(40);
    stats.recordMouseMotion(100, 0);
    QVERIFY(stats.mouseVelocityX() > 0);

    // ...and once they stop, it must decay to zero instead of freezing.
    QTest::qWait(1600);
    QCOMPARE(stats.mouseVelocityX(), 0);
    QCOMPARE(stats.mouseVelocityY(), 0);

    // reset() also clears the gauge.
    stats.recordMouseMotion(-200, 50);
    stats.reset();
    QCOMPARE(stats.mouseVelocityX(), 0);
    QCOMPARE(stats.mouseVelocityY(), 0);
}

QTEST_MAIN(TestKeyStats)
#include "test_keystats.moc"
