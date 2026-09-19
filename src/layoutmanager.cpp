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
#include "layoutmanager.h"
#include <QDir>
#include <QFile>
#include <QDebug>

LayoutManager::LayoutManager(QObject* parent)
    : QObject(parent)
{
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &LayoutManager::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &LayoutManager::onFileChanged);

    // Editors write non-atomically or replace files; wait for the write to
    // settle before reparsing, and QFileSystemWatcher may drop the watch on
    // a replaced file, so reloadCurrentFile() re-arms it every time.
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(300);
    connect(m_debounce, &QTimer::timeout, this, &LayoutManager::reloadCurrentFile);
}

void LayoutManager::bindLayout(KeyLayout* layout) {
    m_layout = layout;
}

void LayoutManager::setLayoutDir(const QString& dir) {
    if (!m_dir.isEmpty()) {
        m_watcher->removePath(m_dir);
    }
    m_dir = dir;
    m_watcher->addPath(dir);
    rescan();
}

QStringList LayoutManager::availableLayouts() const {
    return m_knownFiles;
}

bool LayoutManager::loadLayout(const QString& path) {
    if (!m_layout) {
        return false;
    }

    if (m_layout->loadFromFile(path)) {
        if (m_currentPath != path) {
            if (!m_currentPath.isEmpty()) {
                m_watcher->removePath(m_currentPath);
            }
            m_currentPath = path;
            m_watcher->addPath(path);
        }
        emit layoutLoaded(path);
        return true;
    }

    emit layoutLoadFailed(path);
    return false;
}

void LayoutManager::onDirectoryChanged(const QString& path) {
    Q_UNUSED(path);
    rescan();
}

void LayoutManager::onFileChanged(const QString& path) {
    if (path == m_currentPath) {
        m_debounce->start();
    }
}

void LayoutManager::reloadCurrentFile() {
    if (m_currentPath.isEmpty() || !m_layout) {
        return;
    }

    // The editor may have replaced the file; re-arm the watch either way.
    m_watcher->removePath(m_currentPath);
    if (QFile::exists(m_currentPath)) {
        m_watcher->addPath(m_currentPath);
    }

    if (!m_layout->loadFromFile(m_currentPath)) {
        qWarning() << "Hot reload failed, keeping last good layout:" << m_currentPath;
        emit layoutLoadFailed(m_currentPath);
        return;
    }
    emit layoutLoaded(m_currentPath);
}

void LayoutManager::rescan() {
    QDir dir(m_dir);
    QStringList files;
    for (const QString& fileName : dir.entryList(QStringList() << "*.json", QDir::Files | QDir::Readable)) {
        files.append(dir.absoluteFilePath(fileName));
    }
    files.sort();

    if (files != m_knownFiles) {
        m_knownFiles = files;
        emit layoutListChanged();
    }
}
