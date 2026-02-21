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
#ifndef KEYSTATS_H
#define KEYSTATS_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QTimer>

class KeyStats : public QObject {
    Q_OBJECT

public:
    explicit KeyStats(QObject* parent = nullptr);

    void recordKeyPress(int vkCode);
    void recordKeyRelease(int vkCode);
    void setValidKeys(const QSet<int>& validKeys);

    int totalKeyPresses() const { return m_totalKeyPresses; }
    int kps() const { return m_kps; }
    const QMap<int, int>& keyCounts() const { return m_keyCounts; }
    const QSet<int>& pressedKeys() const { return m_pressedKeys; }

    QVariantMap getStatsJson() const;
    void reset();

signals:
    void statsUpdated();

private slots:
    void updateKps();

private:
    QMap<int, int> m_keyCounts;
    QSet<int> m_pressedKeys;
    QSet<int> m_validKeys;
    QList<qint64> m_recentKeyPressTimes;
    int m_totalKeyPresses = 0;
    int m_kps = 0;
    int m_kpsInstant = 0;
    QTimer* m_kpsTimer = nullptr;
};

#endif
