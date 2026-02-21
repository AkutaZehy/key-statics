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
#include "previewwindow.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QDebug>

PreviewWindow::PreviewWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("key-statics Preview");
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
    
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    QHBoxLayout* topLayout = new QHBoxLayout();
    QLabel* layoutLabel = new QLabel("Layout:", this);
    m_layoutCombo = new QComboBox(this);
    m_resetButton = new QPushButton("Reset", this);
    m_closeButton = new QPushButton("Close", this);
    
    topLayout->addWidget(layoutLabel);
    topLayout->addWidget(m_layoutCombo);
    topLayout->addWidget(m_resetButton);
    topLayout->addWidget(m_closeButton);
    topLayout->addStretch();
    
    m_layout = new KeyLayout(this);
    m_keyboard = new VirtualKeyboard(this);
    m_keyboard->setLayout(m_layout);
    
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_keyboard);
    
    setCentralWidget(centralWidget);
    
    m_keyboardHook = new KeyboardHook(this);
    connect(m_keyboardHook, &KeyboardHook::keyPressed, this, &PreviewWindow::onKeyPressed);
    connect(m_keyboardHook, &KeyboardHook::keyReleased, this, &PreviewWindow::onKeyReleased);
    
    m_mouseHook = new MouseHook(this);
    connect(m_mouseHook, &MouseHook::buttonPressed, this, &PreviewWindow::onMousePressed);
    connect(m_mouseHook, &MouseHook::buttonReleased, this, &PreviewWindow::onMouseReleased);
    
    connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PreviewWindow::onLayoutChanged);
    connect(m_resetButton, &QPushButton::clicked, this, &PreviewWindow::onResetClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &PreviewWindow::close);
    
    loadLayouts();
    
    if (m_keyboardHook->start()) {
        qDebug() << "Preview: keyboard hook started";
    }
    
    if (m_mouseHook->start()) {
        qDebug() << "Preview: mouse hook started";
    }
}

PreviewWindow::~PreviewWindow() {
    if (m_keyboardHook) {
        m_keyboardHook->stop();
    }
    if (m_mouseHook) {
        m_mouseHook->stop();
    }
}

void PreviewWindow::loadLayouts() {
    QString layoutDir = QApplication::applicationDirPath() + "/layouts";
    QDir dir(layoutDir);
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files | QDir::Readable);
    
    m_layoutFiles.clear();
    m_layoutCombo->clear();
    
    for (const QString& fileName : jsonFiles) {
        m_layoutFiles.append(layoutDir + "/" + fileName);
        m_layoutCombo->addItem(fileName);
    }
    
    if (!m_layoutFiles.isEmpty()) {
        loadSelectedLayout();
    }
}

void PreviewWindow::loadSelectedLayout() {
    int index = m_layoutCombo->currentIndex();
    if (index >= 0 && index < m_layoutFiles.size()) {
        QString layoutFile = m_layoutFiles[index];
        if (m_layout->loadFromFile(layoutFile)) {
            m_keyboard->setLayout(m_layout);
            adjustSize();
            
            QFileInfo fileInfo(layoutFile);
            setWindowTitle(QString("key-statics Preview - %1").arg(fileInfo.fileName()));
        }
    }
}

void PreviewWindow::onLayoutChanged(int index) {
    Q_UNUSED(index);
    loadSelectedLayout();
}

void PreviewWindow::onResetClicked() {
    m_keyboard->updatePressedKeys(QSet<int>());
    m_keyboard->repaint();
}

void PreviewWindow::onKeyPressed(int vkCode) {
    m_keyboard->onKeyPressed(vkCode);
}

void PreviewWindow::onKeyReleased(int vkCode) {
    m_keyboard->onKeyReleased(vkCode);
}

void PreviewWindow::onMousePressed(int vkCode) {
    m_keyboard->onKeyPressed(vkCode);
}

void PreviewWindow::onMouseReleased(int vkCode) {
    m_keyboard->onKeyReleased(vkCode);
}
