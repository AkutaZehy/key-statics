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
    void recordMouseMotion(int dx, int dy);
    void recordGamepadAxes(int lt, int rt, int lx, int ly, int rx, int ry);
    void setValidKeys(const QSet<int>& validKeys);

    int totalKeyPresses() const { return m_totalKeyPresses; }
    int kps() const { return m_kps; }
    // Smoothed mouse velocity in px/s; decays to 0 when motion events stop.
    int mouseVelocityX() const;
    int mouseVelocityY() const;
    // Raw analog channels; valid once a controller has reported state.
    bool gamepadConnected() const { return m_gamepadConnected; }
    int padLt() const { return m_padLt; }
    int padRt() const { return m_padRt; }
    int padLx() const { return m_padLx; }
    int padLy() const { return m_padLy; }
    int padRx() const { return m_padRx; }
    int padRy() const { return m_padRy; }
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
    double m_mouseVx = 0.0;
    double m_mouseVy = 0.0;
    qint64 m_lastMotionMs = 0;
    bool m_gamepadConnected = false;
    int m_padLt = 0;
    int m_padRt = 0;
    int m_padLx = 0;
    int m_padLy = 0;
    int m_padRx = 0;
    int m_padRy = 0;
    QTimer* m_kpsTimer = nullptr;
};

#endif
