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
#include "config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QApplication>

Config* Config::s_instance = nullptr;

Config::Config(QObject* parent)
    : QObject(parent)
{
    setDefaults();
}

Config* Config::instance() {
    if (!s_instance) {
        s_instance = new Config();
    }
    return s_instance;
}

void Config::setDefaults() {
    m_serverPort = 9876;
    m_autoPortIfOccupied = true;
    m_unitWidth = 40;
    m_unitHeight = 40;
    m_keySpacing = 4;
    m_backgroundColor = "#282828";
    m_keyColor = "#444444";
    m_keyActiveColor = "#0096FF";
    m_fontFamily = "monospace";
    m_defaultLayout = "104keys";
}

void Config::load(const QString& filePath) {
    QString configFile = filePath;
    if (configFile.isEmpty()) {
        configFile = QApplication::applicationDirPath() + "/config.json";
    }
    
    QFile file(configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Config file not found, using defaults:" << configFile;
        save(configFile);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid config JSON, using defaults";
        return;
    }
    
    loadFromJson(doc.object());
    qDebug() << "Config loaded from:" << configFile;
}

void Config::loadFromJson(const QJsonObject& json) {
    if (json.contains("server")) {
        QJsonObject server = json["server"].toObject();
        m_serverPort = server["port"].toInt(9876);
        m_autoPortIfOccupied = server["autoPortIfOccupied"].toBool(true);
    }
    
    if (json.contains("display")) {
        QJsonObject display = json["display"].toObject();
        m_unitWidth = display["unitWidth"].toInt(40);
        m_unitHeight = display["unitHeight"].toInt(40);
        m_keySpacing = display["keySpacing"].toInt(4);
        m_backgroundColor = display["backgroundColor"].toString("#282828");
        m_keyColor = display["keyColor"].toString("#444444");
        m_keyActiveColor = display["keyActiveColor"].toString("#0096FF");
        m_fontFamily = display["fontFamily"].toString("monospace");
    }
    
    if (json.contains("layout")) {
        QJsonObject layout = json["layout"].toObject();
        m_defaultLayout = layout["default"].toString("104keys");
    }
}

void Config::save(const QString& filePath) {
    QString configFile = filePath;
    if (configFile.isEmpty()) {
        configFile = QApplication::applicationDirPath() + "/config.json";
    }
    
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write config file:" << configFile;
        return;
    }
    
    QJsonDocument doc(saveToJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Config saved to:" << configFile;
}

QJsonObject Config::saveToJson() const {
    QJsonObject json;
    
    QJsonObject server;
    server["port"] = m_serverPort;
    server["autoPortIfOccupied"] = m_autoPortIfOccupied;
    json["server"] = server;
    
    QJsonObject display;
    display["unitWidth"] = m_unitWidth;
    display["unitHeight"] = m_unitHeight;
    display["keySpacing"] = m_keySpacing;
    display["backgroundColor"] = m_backgroundColor;
    display["keyColor"] = m_keyColor;
    display["keyActiveColor"] = m_keyActiveColor;
    display["fontFamily"] = m_fontFamily;
    json["display"] = display;
    
    QJsonObject layout;
    layout["default"] = m_defaultLayout;
    json["layout"] = layout;
    
    return json;
}
