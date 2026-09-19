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
#include "virtualkeyboard.h"
#include "inputconsts.h"
#include "config.h"
#include <QPainter>
#include <cmath>
#include <QDebug>

VirtualKeyboard::VirtualKeyboard(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    m_keyNormalColor = QColor(60, 60, 60, 200);
    m_keyPressedColor = QColor(0, 150, 255, 230);
    m_keyBorderColor = QColor(100, 100, 100);
    m_textColor = Qt::white;

    m_gaugeDecayTimer = new QTimer(this);
    m_gaugeDecayTimer->setInterval(50);
    connect(m_gaugeDecayTimer, &QTimer::timeout, this, &VirtualKeyboard::decayGauge);
}

QSize VirtualKeyboard::sizeHint() const {
    if (m_layout && m_layout->keys().size() > 0) {
        int maxX = 0, maxY = 0;
        auto it = m_layout->keys().constBegin();
        while (it != m_layout->keys().constEnd()) {
            const KeyInfo& info = it.value();
            maxX = qMax(maxX, info.geometry.right() + 20);
            maxY = qMax(maxY, info.geometry.bottom() + 20);
            ++it;
        }
        return QSize(maxX + 30, maxY + 30);
    }
    return QSize(800, 400);
}

void VirtualKeyboard::setLayout(KeyLayout* layout) {
    m_layout = layout;
    if (m_layout && m_layout->keys().size() > 0) {
        int maxX = 0, maxY = 0;
        auto it = m_layout->keys().constBegin();
        while (it != m_layout->keys().constEnd()) {
            const KeyInfo& info = it.value();
            maxX = qMax(maxX, info.geometry.right() + 10);
            maxY = qMax(maxY, info.geometry.bottom() + 10);
            ++it;
        }
        int w = maxX + 30;
        int h = maxY + 30;
        resize(w, h);
    }
    // Our sizeHint changed: invalidate the cached hints of parent layouts,
    // otherwise adjustSize() on the owning window computes from the stale
    // value and the new layout gets clipped.
    updateGeometry();
    update();
}

void VirtualKeyboard::onKeyPressed(int vkCode) {
    m_pressedKeys.insert(vkCode);
    if (m_keyCounts.contains(vkCode)) {
        m_keyCounts[vkCode]++;
    } else {
        m_keyCounts[vkCode] = 1;
    }
    update();
}

void VirtualKeyboard::onKeyReleased(int vkCode) {
    m_pressedKeys.remove(vkCode);
    update();
}

void VirtualKeyboard::updatePressedKeys(const QSet<int>& keys) {
    m_pressedKeys = keys;
    update();
}

void VirtualKeyboard::onMouseMotion(int dx, int dy) {
    const double alpha = 0.45;
    m_gaugeVx = alpha * dx + (1 - alpha) * m_gaugeVx;
    m_gaugeVy = alpha * dy + (1 - alpha) * m_gaugeVy;
    if (!m_gaugeDecayTimer->isActive()) {
        m_gaugeDecayTimer->start();
    }
    update();
}

void VirtualKeyboard::decayGauge() {
    m_gaugeVx *= 0.8;
    m_gaugeVy *= 0.8;
    if (qAbs(m_gaugeVx) < 0.1 && qAbs(m_gaugeVy) < 0.1) {
        m_gaugeVx = 0.0;
        m_gaugeVy = 0.0;
        m_gaugeDecayTimer->stop();
    }
    update();
}

void VirtualKeyboard::onGamepadAxes(int lt, int rt, int lx, int ly, int rx, int ry) {
    Q_UNUSED(lx);
    Q_UNUSED(ly);
    Q_UNUSED(rx);
    Q_UNUSED(ry);
    m_padLt = lt;
    m_padRt = rt;
    update();
}

void VirtualKeyboard::drawGauge(QPainter* painter, const QRect& rect) const {
    painter->save();

    painter->setBrush(m_keyNormalColor);
    painter->setPen(m_keyBorderColor);
    painter->drawRoundedRect(rect, 8, 8);

    const QPointF center = rect.center();
    const double maxR = qMin(rect.width(), rect.height()) / 2.0 - 8.0;

    QPen ringPen(QColor(255, 255, 255, 30));
    ringPen.setWidthF(1.0);
    painter->setPen(ringPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(center, maxR, maxR);
    painter->drawEllipse(center, maxR * 0.5, maxR * 0.5);

    // m_gaugeVx is px per motion sample; normalize the same way the web
    // gauge does: convert to px/s, then clamp against gaugeMaxSpeed.
    const double scale = Config::instance()->gaugeMaxSpeed();
    double nx = (m_gaugeVx * (1000.0 / MOUSE_MOTION_SAMPLE_MS)) / scale;
    double ny = (m_gaugeVy * (1000.0 / MOUSE_MOTION_SAMPLE_MS)) / scale;
    const double mag = std::hypot(nx, ny);
    if (mag > 1.0) {
        nx /= mag;
        ny /= mag;
    }

    QPointF tip = center + QPointF(nx, ny) * maxR;
    QPen arrowPen(m_keyPressedColor);
    arrowPen.setWidthF(2.0);
    painter->setPen(arrowPen);
    painter->setBrush(m_keyPressedColor);
    painter->drawLine(center, tip);

    const double angle = std::atan2(ny, nx);
    QPolygonF head;
    head << tip
         << tip + QPointF(std::cos(angle + M_PI - 0.45), std::sin(angle + M_PI - 0.45)) * 9.0
         << tip + QPointF(std::cos(angle + M_PI + 0.45), std::sin(angle + M_PI + 0.45)) * 9.0;
    painter->drawPolygon(head);

    painter->setPen(Qt::NoPen);
    painter->setBrush(m_textColor);
    painter->drawEllipse(center, 2.5, 2.5);

    painter->restore();
}

void VirtualKeyboard::drawTriggerBars(QPainter* painter, const QRect& rect) const {
    painter->save();

    painter->setBrush(m_keyNormalColor);
    painter->setPen(m_keyBorderColor);
    painter->drawRoundedRect(rect, 8, 8);

    const int pad = 10;
    const int barWidth = (rect.width() - pad * 3) / 2;
    const int usableHeight = rect.height() - pad * 2 - 14;

    QFont font = painter->font();
    font.setPixelSize(11);
    painter->setFont(font);

    const int triggers[2] = {m_padLt, m_padRt};
    const QString labels[2] = {"LT", "RT"};
    for (int i = 0; i < 2; ++i) {
        const int x = rect.left() + pad + i * (barWidth + pad);
        const int trackTop = rect.top() + pad;
        const int trackHeight = usableHeight;

        painter->setPen(QPen(m_keyBorderColor, 1));
        painter->setBrush(QColor(0, 0, 0, 80));
        painter->drawRoundedRect(QRect(x, trackTop, barWidth, trackHeight), 4, 4);

        const int fillHeight = qBound(0, triggers[i] * trackHeight / 255, trackHeight);
        if (fillHeight > 0) {
            painter->setBrush(m_keyPressedColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(QRect(x, trackTop + trackHeight - fillHeight, barWidth, fillHeight), 4, 4);
        }

        painter->setPen(m_textColor);
        painter->drawText(QRect(x, trackTop + trackHeight + 2, barWidth, 12), Qt::AlignCenter, labels[i]);
    }

    painter->restore();
}

void VirtualKeyboard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_layout) {
        return;
    }

    auto it = m_layout->keys().constBegin();
    while (it != m_layout->keys().constEnd()) {
        const KeyInfo& info = it.value();
        QRect rect = info.geometry;
        rect.translate(10, 10);

        if (info.isVirtualElement()) {
            if (info.vkCode == VK_GAUGE_MOUSE_VELOCITY) {
                drawGauge(&painter, rect);
            } else if (info.vkCode == VK_GAUGE_GAMEPAD_TRIGGERS) {
                drawTriggerBars(&painter, rect);
            }
            ++it;
            continue;
        }

        bool pressed = m_pressedKeys.contains(info.vkCode);
        QColor bgColor = pressed ? m_keyPressedColor : m_keyNormalColor;

        painter.setBrush(bgColor);
        painter.setPen(m_keyBorderColor);
        painter.drawRoundedRect(rect, 6, 6);

        painter.setPen(m_textColor);
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(14);
        painter.setFont(font);
        painter.drawText(rect, Qt::AlignCenter, info.label);
        ++it;
    }
}
