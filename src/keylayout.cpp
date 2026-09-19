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
#include "keylayout.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

KeyLayout::KeyLayout(QObject* parent)
    : QObject(parent)
    , m_unitWidth(40)
    , m_unitHeight(40)
    , m_keySpacing(4)
{
}

bool KeyLayout::loadFromFile(const QString& filePath) {
    // Parse into locals first: on failure the currently active layout must
    // stay untouched (hot reload keeps serving the last good layout).
    QString parsedName;
    QMap<int, KeyInfo> parsedKeys;
    int parsedUnitWidth = 40;
    int parsedUnitHeight = 40;
    int parsedKeySpacing = 4;

    if (!parseFile(filePath, parsedName, parsedKeys, parsedUnitWidth, parsedUnitHeight, parsedKeySpacing)) {
        return false;
    }

    m_name = parsedName;
    m_keys = parsedKeys;
    m_unitWidth = parsedUnitWidth;
    m_unitHeight = parsedUnitHeight;
    m_keySpacing = parsedKeySpacing;

    qDebug() << "Loaded layout:" << m_name << "with" << m_keys.size() << "keys";
    return true;
}

bool KeyLayout::parseFile(const QString& filePath, QString& name, QMap<int, KeyInfo>& keys,
                          int& unitWidth, int& unitHeight, int& keySpacing) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open layout file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON format in layout file:" << filePath;
        return false;
    }

    QJsonObject root = doc.object();
    name = root.value("name").toString("Unknown");
    unitWidth = root.value("unitWidth").toInt(40);
    unitHeight = root.value("unitHeight").toInt(40);
    keySpacing = root.value("keySpacing").toInt(4);

    if (unitWidth <= 0 || unitHeight <= 0 || keySpacing < 0) {
        qWarning() << "Invalid layout dimensions in:" << filePath;
        return false;
    }

    QJsonArray keysArray = root.value("keys").toArray();
    keys.clear();

    for (const QJsonValue& keyValue : keysArray) {
        QJsonObject keyObj = keyValue.toObject();
        KeyInfo info;
        info.vkCode = keyObj.value("vkCode").toInt(0);
        info.label = keyObj.value("label").toString("");
        info.row = keyObj.value("row").toDouble(0);
        info.col = keyObj.value("col").toDouble(0);
        info.width = keyObj.value("width").toDouble(1);
        info.height = keyObj.value("height").toDouble(1);

        int x = static_cast<int>(info.col * (unitWidth + keySpacing));
        int y = static_cast<int>(info.row * (unitHeight + keySpacing));
        int w = static_cast<int>(info.width * unitWidth + (info.width - 1) * keySpacing);
        int h = static_cast<int>(info.height * unitHeight + (info.height - 1) * keySpacing);
        info.geometry = QRect(x, y, w, h);

        keys.insert(info.vkCode, info);
    }

    return true;
}

QRect KeyLayout::getKeyGeometry(int vkCode) const {
    auto it = m_keys.find(vkCode);
    if (it != m_keys.end()) {
        return it.value().geometry;
    }
    return QRect();
}

QString KeyLayout::getKeyLabel(int vkCode) const {
    auto it = m_keys.find(vkCode);
    if (it != m_keys.end()) {
        return it.value().label;
    }
    return QString();
}
