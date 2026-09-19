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
#include "config.h"

namespace {

bool writeTempFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(content) == content.size();
}

} // namespace

class TestConfig : public QObject {
    Q_OBJECT

private slots:
    // Declaration order = execution order: state-mutating tests must run
    // after the ones that rely on the fresh (default) singleton state.
    void missingFileUsesDefaults();
    void invalidJsonUsesDefaults();
    void loadCustomValues();
    void saveRoundtrip();

private:
    QTemporaryDir m_dir;
};

void TestConfig::loadCustomValues() {
    QString path = m_dir.path() + "/custom.json";
    QVERIFY(writeTempFile(path, R"({
        "server": {"port": 1234, "autoPortIfOccupied": false, "allowRemoteAccess": true},
        "display": {"unitWidth": 50, "unitHeight": 55, "keySpacing": 6,
                    "backgroundColor": "#111111", "keyColor": "#222222",
                    "keyActiveColor": "#333333", "fontFamily": "Arial"},
        "layout": {"default": "wasd"}
    })"));

    // Config is a singleton; loading a full file fully determines the state.
    Config::instance()->load(path);

    QCOMPARE(Config::instance()->serverPort(), quint16(1234));
    QCOMPARE(Config::instance()->autoPortIfOccupied(), false);
    QCOMPARE(Config::instance()->allowRemoteAccess(), true);
    QCOMPARE(Config::instance()->unitWidth(), 50);
    QCOMPARE(Config::instance()->unitHeight(), 55);
    QCOMPARE(Config::instance()->keySpacing(), 6);
    QCOMPARE(Config::instance()->backgroundColor(), QString("#111111"));
    QCOMPARE(Config::instance()->keyColor(), QString("#222222"));
    QCOMPARE(Config::instance()->keyActiveColor(), QString("#333333"));
    QCOMPARE(Config::instance()->fontFamily(), QString("Arial"));
    QCOMPARE(Config::instance()->defaultLayout(), QString("wasd"));
}

void TestConfig::missingFileUsesDefaults() {
    QString path = m_dir.path() + "/missing.json";

    // First run: nothing on disk, the singleton still holds defaults and
    // they get written out as a starting config.
    Config::instance()->load(path);

    QCOMPARE(Config::instance()->serverPort(), quint16(9876));
    QCOMPARE(Config::instance()->autoPortIfOccupied(), true);
    QCOMPARE(Config::instance()->allowRemoteAccess(), false);
    QCOMPARE(Config::instance()->unitWidth(), 40);
    QCOMPARE(Config::instance()->defaultLayout(), QString("104keys"));
    QVERIFY(QFile::exists(path));
}

void TestConfig::invalidJsonUsesDefaults() {
    QString path = m_dir.path() + "/broken.json";
    QVERIFY(writeTempFile(path, "{ not valid json"));

    Config::instance()->load(path);

    QCOMPARE(Config::instance()->serverPort(), quint16(9876));
    QCOMPARE(Config::instance()->allowRemoteAccess(), false);
}

void TestConfig::saveRoundtrip() {
    QString path = m_dir.path() + "/roundtrip.json";
    QVERIFY(writeTempFile(path, R"({
        "server": {"port": 4321, "autoPortIfOccupied": false, "allowRemoteAccess": true},
        "display": {"unitWidth": 60, "unitHeight": 60, "keySpacing": 2,
                    "backgroundColor": "#000000", "keyColor": "#111111",
                    "keyActiveColor": "#222222", "fontFamily": "Consolas"},
        "layout": {"default": "dfjk"}
    })"));

    Config::instance()->load(path);
    Config::instance()->save(path);
    Config::instance()->load(path);

    QCOMPARE(Config::instance()->serverPort(), quint16(4321));
    QCOMPARE(Config::instance()->autoPortIfOccupied(), false);
    QCOMPARE(Config::instance()->allowRemoteAccess(), true);
    QCOMPARE(Config::instance()->unitWidth(), 60);
    QCOMPARE(Config::instance()->keySpacing(), 2);
    QCOMPARE(Config::instance()->fontFamily(), QString("Consolas"));
    QCOMPARE(Config::instance()->defaultLayout(), QString("dfjk"));
}

QTEST_MAIN(TestConfig)
#include "test_config.moc"
