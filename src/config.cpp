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
#include <QJsonArray>
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
    m_allowRemoteAccess = false;
    m_unitWidth = 40;
    m_unitHeight = 40;
    m_keySpacing = 4;
    m_backgroundColor = "#282828";
    m_keyColor = "#444444";
    m_keyActiveColor = "#0096FF";
    m_fontFamily = "monospace";
    m_gaugeMaxSpeed = 3000;
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
        m_allowRemoteAccess = server["allowRemoteAccess"].toBool(false);
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
        m_gaugeMaxSpeed = display["gaugeMaxSpeed"].toInt(3000);
    }

    if (json.contains("layout")) {
        QJsonObject layout = json["layout"].toObject();
        m_defaultLayout = layout["default"].toString("104keys");

        m_autoSwitchRules.clear();
        const QJsonArray rules = layout["autoSwitch"].toArray();
        for (const QJsonValue& value : rules) {
            QJsonObject rule = value.toObject();
            QString process = rule["process"].toString().trimmed();
            QString layoutName = rule["layout"].toString().trimmed();
            if (!process.isEmpty() && !layoutName.isEmpty()) {
                m_autoSwitchRules.append(qMakePair(process, layoutName));
            }
        }
    }

    if (json.contains("gamepad")) {
        QJsonObject gamepad = json["gamepad"].toObject();
        m_gamepadEnabled = gamepad["enabled"].toBool(true);
        m_gamepadUserIndex = gamepad["userIndex"].toInt(0);
        if (m_gamepadUserIndex < 0 || m_gamepadUserIndex > 3) {
            m_gamepadUserIndex = 0;
        }
    }
}

QString Config::matchAutoSwitch(const QString& exeBaseName) const {
    for (const auto& rule : m_autoSwitchRules) {
        if (exeBaseName.contains(rule.first, Qt::CaseInsensitive)) {
            return rule.second;
        }
    }
    return QString();
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
    server["allowRemoteAccess"] = m_allowRemoteAccess;
    json["server"] = server;
    
    QJsonObject display;
    display["unitWidth"] = m_unitWidth;
    display["unitHeight"] = m_unitHeight;
    display["keySpacing"] = m_keySpacing;
    display["backgroundColor"] = m_backgroundColor;
    display["keyColor"] = m_keyColor;
    display["keyActiveColor"] = m_keyActiveColor;
    display["fontFamily"] = m_fontFamily;
    display["gaugeMaxSpeed"] = m_gaugeMaxSpeed;
    json["display"] = display;
    
    QJsonObject layout;
    layout["default"] = m_defaultLayout;
    QJsonArray rules;
    for (const auto& rule : m_autoSwitchRules) {
        QJsonObject ruleJson;
        ruleJson["process"] = rule.first;
        ruleJson["layout"] = rule.second;
        rules.append(ruleJson);
    }
    if (!rules.isEmpty()) {
        layout["autoSwitch"] = rules;
    }
    json["layout"] = layout;

    QJsonObject gamepad;
    gamepad["enabled"] = m_gamepadEnabled;
    gamepad["userIndex"] = m_gamepadUserIndex;
    json["gamepad"] = gamepad;

    return json;
}
