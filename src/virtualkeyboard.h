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
#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QWidget>
#include <QSet>
#include <QMap>
#include <QSize>
#include "keylayout.h"

class VirtualKeyboard : public QWidget {
    Q_OBJECT

public:
    explicit VirtualKeyboard(QWidget* parent = nullptr);
    void setLayout(KeyLayout* layout);

    QSize sizeHint() const override;

public slots:
    void onKeyPressed(int vkCode);
    void onKeyReleased(int vkCode);
    void updatePressedKeys(const QSet<int>& keys);

signals:
    void keyClicked(int vkCode);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    KeyLayout* m_layout = nullptr;
    QSet<int> m_pressedKeys;
    QMap<int, int> m_keyCounts;

    QColor m_keyNormalColor;
    QColor m_keyPressedColor;
    QColor m_keyBorderColor;
    QColor m_textColor;
};

#endif
