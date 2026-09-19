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
#ifndef KEYLAYOUT_H
#define KEYLAYOUT_H

#include <QObject>
#include <QMap>
#include <QRect>
#include <QString>
#include "inputconsts.h"

struct KeyInfo {
    int vkCode = 0;
    QString label;
    QRect geometry;
    double row = 0.0;
    double col = 0.0;
    double width = 1.0;
    double height = 1.0;

    // Virtual elements (gauges etc.) occupy layout space but render as
    // instruments, not keycaps, and never receive press events.
    bool isVirtualElement() const { return vkCode >= VK_ELEMENT_FIRST; }
};

class KeyLayout : public QObject {
    Q_OBJECT

public:
    explicit KeyLayout(QObject* parent = nullptr);

    bool loadFromFile(const QString& filePath);
    const QMap<int, KeyInfo>& keys() const { return m_keys; }
    const QString& name() const { return m_name; }
    int unitWidth() const { return m_unitWidth; }
    int unitHeight() const { return m_unitHeight; }
    int keySpacing() const { return m_keySpacing; }
    QRect getKeyGeometry(int vkCode) const;
    QString getKeyLabel(int vkCode) const;

private:
    bool parseFile(const QString& filePath, QString& name, QMap<int, KeyInfo>& keys,
                   int& unitWidth, int& unitHeight, int& keySpacing) const;
    QMap<int, KeyInfo> m_keys;
    QString m_name;
    int m_unitWidth = 40;
    int m_unitHeight = 40;
    int m_keySpacing = 4;
};

#endif
