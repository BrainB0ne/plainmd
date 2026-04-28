/*
 * PlainMD
 * Copyright (C) 2026 BrainByteZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MINIMAP_H
#define MINIMAP_H

#include <QWidget>

class QTextEdit;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;

class Minimap : public QWidget
{
    Q_OBJECT

public:
    explicit Minimap(QTextEdit *editor, QWidget *parent = nullptr);

    void updateContent();
    void setPlainTextMode(bool enabled);  // Enable for .txt files to skip markdown detection

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onEditorScroll();
    void onEditorTextChanged();

private:
    void calculateScale();
    qreal getDocumentHeight() const;
    QRectF getViewportRect() const;
    void scrollEditorToPosition(qreal relativeY);
    bool isDarkTheme() const;

    QTextEdit *m_editor = nullptr;
    qreal m_scale = 0.15;  // Default scale factor (15%)
    int m_minimapWidth = 120;  // Fixed width for minimap
    bool m_dragging = false;
    bool m_plainTextMode = false;  // True for .txt files
};

#endif // MINIMAP_H
