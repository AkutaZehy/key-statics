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

struct KeyInfo {
    int vkCode;
    QString label;
    QRect geometry;
    double row;
    double col;
    double width;
    double height;
};

class KeyLayout : public QObject {
    Q_OBJECT

public:
    explicit KeyLayout(QObject* parent = nullptr);

    bool loadFromFile(const QString& filePath);
    const QMap<int, KeyInfo>& keys() const { return m_keys; }
    const QString& name() const { return m_name; }

    QRect getKeyGeometry(int vkCode) const;
    QString getKeyLabel(int vkCode) const;

private:
    QMap<int, KeyInfo> m_keys;
    QString m_name;
    int m_unitWidth = 40;
    int m_unitHeight = 40;
    int m_keySpacing = 4;
};

#endif
