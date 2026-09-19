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
#include <QTemporaryDir>
#include "keylayout.h"
#include "inputconsts.h"

namespace {

bool writeTempFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(content) == content.size();
}

} // namespace

class TestKeyLayout : public QObject {
    Q_OBJECT

private slots:
    void loadValidLayout();
    void defaultsApplied();
    void missingFileFails();
    void invalidJsonFails();
    void loadsVirtualElements();

private:
    QTemporaryDir m_dir;
};

void TestKeyLayout::loadValidLayout() {
    QString path = m_dir.path() + "/valid.json";
    QVERIFY(writeTempFile(path, R"({
        "name": "Test Layout",
        "unitWidth": 40,
        "unitHeight": 40,
        "keySpacing": 4,
        "keys": [
            {"vkCode": 68, "label": "D", "row": 0, "col": 0, "width": 2},
            {"vkCode": 70, "label": "F", "row": 1.5, "col": 2.5}
        ]
    })"));

    KeyLayout layout;
    QVERIFY(layout.loadFromFile(path));

    QCOMPARE(layout.name(), QString("Test Layout"));
    QCOMPARE(layout.keys().size(), 2);

    const KeyInfo wideKey = layout.keys().value(68);
    QCOMPARE(wideKey.label, QString("D"));
    QCOMPARE(wideKey.width, 2.0);
    QCOMPARE(wideKey.height, 1.0);
    QCOMPARE(wideKey.geometry, QRect(0, 0, 2 * 40 + 1 * 4, 40));

    const KeyInfo decimalKey = layout.keys().value(70);
    QCOMPARE(decimalKey.row, 1.5);
    QCOMPARE(decimalKey.col, 2.5);
    QCOMPARE(decimalKey.geometry, QRect(static_cast<int>(2.5 * 44), static_cast<int>(1.5 * 44), 40, 40));
}

void TestKeyLayout::defaultsApplied() {
    QString path = m_dir.path() + "/defaults.json";
    QVERIFY(writeTempFile(path, R"({"keys": [{"vkCode": 75, "label": "K", "row": 0, "col": 0}]})"));

    KeyLayout layout;
    QVERIFY(layout.loadFromFile(path));

    QCOMPARE(layout.name(), QString("Unknown"));
    QCOMPARE(layout.unitWidth(), 40);
    QCOMPARE(layout.unitHeight(), 40);
    QCOMPARE(layout.keySpacing(), 4);
    QCOMPARE(layout.keys().value(75).width, 1.0);
}

void TestKeyLayout::missingFileFails() {
    KeyLayout layout;
    QVERIFY(!layout.loadFromFile(m_dir.path() + "/does-not-exist.json"));
}

void TestKeyLayout::invalidJsonFails() {
    QString path = m_dir.path() + "/broken.json";
    QVERIFY(writeTempFile(path, "{ this is not json"));

    KeyLayout layout;
    QVERIFY(!layout.loadFromFile(path));
}

void TestKeyLayout::loadsVirtualElements() {
    QString path = m_dir.path() + "/gauge.json";
    QVERIFY(writeTempFile(path, R"({
        "unitWidth": 40,
        "unitHeight": 40,
        "keySpacing": 4,
        "keys": [
            {"vkCode": 512, "label": "VEL", "row": 0.5, "col": 22.5, "width": 4, "height": 5},
            {"vkCode": 68, "label": "D", "row": 0, "col": 0}
        ]
    })"));

    KeyLayout layout;
    QVERIFY(layout.loadFromFile(path));

    const KeyInfo& gauge = layout.keys().value(512);
    QCOMPARE(gauge.vkCode, VK_GAUGE_MOUSE_VELOCITY);
    QVERIFY(gauge.isVirtualElement());
    QCOMPARE(gauge.geometry, QRect(static_cast<int>(22.5 * 44), static_cast<int>(0.5 * 44), 4 * 40 + 3 * 4, 5 * 40 + 4 * 4));

    QVERIFY(!layout.keys().value(68).isVirtualElement());
}

QTEST_MAIN(TestKeyLayout)
#include "test_keylayout.moc"
