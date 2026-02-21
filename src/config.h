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
#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class Config : public QObject {
    Q_OBJECT

public:
    static Config* instance();

    void load(const QString& filePath = QString());
    void save(const QString& filePath = QString());

    quint16 serverPort() const { return m_serverPort; }
    bool autoPortIfOccupied() const { return m_autoPortIfOccupied; }
    
    int unitWidth() const { return m_unitWidth; }
    int unitHeight() const { return m_unitHeight; }
    int keySpacing() const { return m_keySpacing; }
    QString backgroundColor() const { return m_backgroundColor; }
    QString keyColor() const { return m_keyColor; }
    QString keyActiveColor() const { return m_keyActiveColor; }
    QString fontFamily() const { return m_fontFamily; }
    
    QString defaultLayout() const { return m_defaultLayout; }

    void setServerPort(quint16 port) { m_serverPort = port; }
    void setDefaultLayout(const QString& layout) { m_defaultLayout = layout; }

private:
    explicit Config(QObject* parent = nullptr);
    void setDefaults();
    void loadFromJson(const QJsonObject& json);
    QJsonObject saveToJson() const;
    
    static Config* s_instance;
    
    quint16 m_serverPort = 9876;
    bool m_autoPortIfOccupied = true;
    
    int m_unitWidth = 40;
    int m_unitHeight = 40;
    int m_keySpacing = 4;
    QString m_backgroundColor = "#282828";
    QString m_keyColor = "#444444";
    QString m_keyActiveColor = "#0096FF";
    QString m_fontFamily = "monospace";
    
    QString m_defaultLayout = "104keys";
};

#endif
