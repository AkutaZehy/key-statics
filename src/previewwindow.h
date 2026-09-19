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
#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "keylayout.h"
#include "virtualkeyboard.h"

class PreviewWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit PreviewWindow(QWidget* parent = nullptr);

public slots:
    // Fed from the single pair of global hooks owned by MainWindow; the
    // preview must not install its own (the hook procs are singletons).
    void onKeyPressed(int vkCode);
    void onKeyReleased(int vkCode);
    void onMousePressed(int vkCode);
    void onMouseReleased(int vkCode);
    void onMouseMotion(int dx, int dy) {
        m_keyboard->onMouseMotion(dx, dy);
    }
    void onGamepadAxes(int lt, int rt, int lx, int ly, int rx, int ry) {
        m_keyboard->onGamepadAxes(lt, rt, lx, ly, rx, ry);
    }
    // Rescans the layouts dir so files added after opening show up.
    void refreshLayouts() {
        loadLayouts();
    }

private slots:
    void onLayoutChanged(int index);
    void onResetClicked();

private:
    void loadLayouts();
    void loadSelectedLayout();
    void updateLastInputLabel(int vkCode);

    QComboBox* m_layoutCombo = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QLabel* m_lastInputLabel = nullptr;

    KeyLayout* m_layout = nullptr;
    VirtualKeyboard* m_keyboard = nullptr;

    QStringList m_layoutFiles;
};

#endif
