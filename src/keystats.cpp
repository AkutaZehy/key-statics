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
#include "keystats.h"
#include "inputconsts.h"
#include <QDateTime>
#include <QJsonObject>

KeyStats::KeyStats(QObject* parent)
    : QObject(parent)
{
    m_kpsTimer = new QTimer(this);
    connect(m_kpsTimer, &QTimer::timeout, this, &KeyStats::updateKps);
    m_kpsTimer->start(100);
}

void KeyStats::setValidKeys(const QSet<int>& validKeys) {
    m_validKeys = validKeys;
}

void KeyStats::recordKeyPress(int vkCode) {
    if (!m_validKeys.isEmpty() && !m_validKeys.contains(vkCode)) {
        return;
    }
    
    m_pressedKeys.insert(vkCode);
    
    if (m_keyCounts.contains(vkCode)) {
        m_keyCounts[vkCode]++;
    } else {
        m_keyCounts[vkCode] = 1;
    }

    m_recentKeyPressTimes.append(QDateTime::currentMSecsSinceEpoch());
    m_totalKeyPresses++;
    
    emit statsUpdated();
}

void KeyStats::recordKeyRelease(int vkCode) {
    if (m_pressedKeys.remove(vkCode)) {
        emit statsUpdated();
    }
}

void KeyStats::recordMouseMotion(int dx, int dy) {
    const double alpha = 0.45;
    m_mouseVx = alpha * dx + (1 - alpha) * m_mouseVx;
    m_mouseVy = alpha * dy + (1 - alpha) * m_mouseVy;
    m_lastMotionMs = QDateTime::currentMSecsSinceEpoch();
    emit statsUpdated();
}

int KeyStats::mouseVelocityX() const {
    return qRound(m_mouseVx * (1000.0 / MOUSE_MOTION_SAMPLE_MS));
}

int KeyStats::mouseVelocityY() const {
    return qRound(m_mouseVy * (1000.0 / MOUSE_MOTION_SAMPLE_MS));
}

void KeyStats::updateKps() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 windowStart = now - 100;

    while (!m_recentKeyPressTimes.isEmpty() && m_recentKeyPressTimes.first() < windowStart) {
        m_recentKeyPressTimes.removeFirst();
    }

    m_kpsInstant = m_recentKeyPressTimes.size() * 10;

    const double alpha = 0.5;
    int newKps = static_cast<int>(alpha * m_kpsInstant + (1 - alpha) * m_kps);
    if (newKps != m_kps) {
        m_kps = newKps;
        emit statsUpdated();
    }

    // Once motion events stop arriving, decay the gauge toward zero instead
    // of freezing at the last measured velocity.
    if (m_lastMotionMs > 0 && now - m_lastMotionMs > MOUSE_MOTION_SAMPLE_MS * 3) {
        int oldVx = mouseVelocityX();
        int oldVy = mouseVelocityY();
        m_mouseVx *= 0.35;
        m_mouseVy *= 0.35;
        if (qAbs(m_mouseVx) < 0.2) m_mouseVx = 0.0;
        if (qAbs(m_mouseVy) < 0.2) m_mouseVy = 0.0;
        if (mouseVelocityX() != oldVx || mouseVelocityY() != oldVy) {
            emit statsUpdated();
        }
    }
}

QVariantMap KeyStats::getStatsJson() const {
    QVariantMap stats;
    stats["totalKeyPresses"] = m_totalKeyPresses;
    stats["kps"] = m_kps;
    
    QVariantMap keyCounts;
    for (auto it = m_keyCounts.constBegin(); it != m_keyCounts.constEnd(); ++it) {
        keyCounts[QString::number(it.key())] = it.value();
    }
    stats["keyCounts"] = keyCounts;
    
    return stats;
}

void KeyStats::reset() {
    m_keyCounts.clear();
    m_pressedKeys.clear();
    m_recentKeyPressTimes.clear();
    m_totalKeyPresses = 0;
    m_kps = 0;
    m_kpsInstant = 0;
    m_mouseVx = 0.0;
    m_mouseVy = 0.0;
    emit statsUpdated();
}
