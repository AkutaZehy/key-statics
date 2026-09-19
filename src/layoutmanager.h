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
#ifndef LAYOUTMANAGER_H
#define LAYOUTMANAGER_H

#include <QObject>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QTimer>
#include "keylayout.h"

// Owns the layouts/ directory view: scanning, change watching and hot
// reload of the currently active layout file. Parses into the shared
// KeyLayout only after a full successful parse, so a broken edit keeps
// the last good layout on screen.
class LayoutManager : public QObject {
    Q_OBJECT

public:
    explicit LayoutManager(QObject* parent = nullptr);

    void bindLayout(KeyLayout* layout);
    void setLayoutDir(const QString& dir);

    QStringList availableLayouts() const;
    QString currentLayoutPath() const { return m_currentPath; }

    // Loads the file into the bound layout and emits layoutLoaded on success.
    bool loadLayout(const QString& path);

signals:
    void layoutListChanged();
    void layoutLoaded(const QString& path);
    void layoutLoadFailed(const QString& path);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);
    void reloadCurrentFile();

private:
    void rescan();

    KeyLayout* m_layout = nullptr;
    QString m_dir;
    QString m_currentPath;
    QStringList m_knownFiles;
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_debounce = nullptr;
};

#endif
