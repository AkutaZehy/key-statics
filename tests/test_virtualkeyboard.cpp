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
#include <QMainWindow>
#include "virtualkeyboard.h"

namespace {
QString layoutPath(const QString& fileName) {
    return QString(KEY_STATICS_SOURCE_DIR) + "/layouts/" + fileName;
}
} // namespace

class TestVirtualKeyboard : public QObject {
    Q_OBJECT

private slots:
    // Regression: switching layouts must invalidate the cached size hint of
    // the parent layout, otherwise the owning window keeps the previous
    // layout's size and the new layout is clipped (seen with gamepad.json
    // inside the preview window).
    void windowSizeHintFollowsLayoutSwitch();
};

void TestVirtualKeyboard::windowSizeHintFollowsLayoutSwitch() {
    KeyLayout layout104;
    KeyLayout layoutGamepad;
    QVERIFY(layout104.loadFromFile(layoutPath("104keys.json")));
    QVERIFY(layoutGamepad.loadFromFile(layoutPath("gamepad.json")));

    // Heap-allocated on purpose: setCentralWidget() takes ownership.
    auto* keyboard = new VirtualKeyboard();
    QMainWindow window;
    window.setCentralWidget(keyboard);

    keyboard->setLayout(&layout104);
    window.adjustSize();
    const QSize hintBefore = window.sizeHint();

    keyboard->setLayout(&layoutGamepad);
    window.adjustSize();
    const QSize hintAfter = window.sizeHint();

    QVERIFY(hintAfter.height() > hintBefore.height());
    int maxY = 0;
    for (const KeyInfo& info : layoutGamepad.keys()) {
        maxY = qMax(maxY, info.geometry.bottom() + 10);
    }
    QVERIFY(hintAfter.height() >= maxY + 30); // full gamepad layout must fit
    // The window hint must fully contain the keyboard's own hint.
    QVERIFY(hintAfter.height() >= keyboard->sizeHint().height());
    QVERIFY(hintAfter.width() >= keyboard->sizeHint().width());
}

QTEST_MAIN(TestVirtualKeyboard)
#include "test_virtualkeyboard.moc"
