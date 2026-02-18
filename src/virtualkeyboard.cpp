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
#include <QPainter>
#include <QDebug>

VirtualKeyboard::VirtualKeyboard(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    m_keyNormalColor = QColor(60, 60, 60, 200);
    m_keyPressedColor = QColor(0, 150, 255, 230);
    m_keyBorderColor = QColor(100, 100, 100);
    m_textColor = Qt::white;
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
