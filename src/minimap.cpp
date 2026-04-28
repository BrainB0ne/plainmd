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

#include "minimap.h"

#include <QTextEdit>
#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextFragment>
#include <QTextBlockFormat>
#include <QTextList>
#include <QCoreApplication>
#include <QApplication>

Minimap::Minimap(QTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , m_editor(editor)
{
    setFixedWidth(m_minimapWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    setAttribute(Qt::WA_TranslucentBackground, false);
    
    if (m_editor) {
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &Minimap::onEditorScroll);
        connect(m_editor->document(), &QTextDocument::contentsChanged,
                this, &Minimap::onEditorTextChanged);
        
        calculateScale();
    }
}

bool Minimap::isDarkTheme() const
{
    // Detect dark theme by checking the button color (for consistency with paint)
    QPalette palette = QApplication::palette();
    QColor buttonColor = palette.color(QPalette::Button);
    return buttonColor.lightness() < 128;
}

void Minimap::updateContent()
{
    calculateScale();
    update();
}

void Minimap::setPlainTextMode(bool enabled)
{
    m_plainTextMode = enabled;
    update();
}

void Minimap::calculateScale()
{
    if (!m_editor || !m_editor->document())
        return;
    
    qreal docHeight = getDocumentHeight();
    if (docHeight > 0 && height() > 0) {
        // Scale to fit entire document within minimap (no minimum, max 0.3)
        m_scale = qMin(height() / docHeight, 0.3);
    }
}

qreal Minimap::getDocumentHeight() const
{
    if (!m_editor || !m_editor->document())
        return 0;
    
    return m_editor->document()->size().height();
}

QRectF Minimap::getViewportRect() const
{
    if (!m_editor)
        return QRectF();
    
    QScrollBar *vScroll = m_editor->verticalScrollBar();
    qreal scrollRatio = 0;
    
    if (vScroll->maximum() > 0) {
        scrollRatio = static_cast<qreal>(vScroll->value()) / vScroll->maximum();
    }
    
    qreal docHeight = getDocumentHeight() * m_scale;
    qreal viewportHeight = m_editor->viewport()->height() * m_scale;
    qreal y = scrollRatio * (docHeight - viewportHeight);
    
    y = qMax(0.0, qMin(y, static_cast<qreal>(height()) - viewportHeight));
    
    return QRectF(2, y, width() - 4, qMax(20.0, viewportHeight));
}

void Minimap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // Use button color for background to blend in with the UI (menu bar color)
    QPalette palette = QApplication::palette();
    QColor bgColor = palette.color(QPalette::Button);
    painter.fillRect(rect(), bgColor);
    
    if (!m_editor || !m_editor->document())
        return;
    
    // Determine if we need dark or light content colors based on background
    bool dark = bgColor.lightness() < 128;
    
    // Mocha (dark background) palette
    QColor mochaImage = QColor(238, 153, 153);     // Soft coral rose - Images
    QColor mochaBlue = QColor(137, 180, 250);      // 89b4fa - H1, highlight
    QColor mochaGreen = QColor(166, 227, 161);     // a6e3a1 - H2
    QColor mochaPeach = QColor(250, 179, 135);     // fab387 - H3
    QColor mochaTeal = QColor(148, 226, 213);      // 94e2d5 - Links
    QColor mochaYellow = QColor(249, 226, 175);    // f9e2af - Lists
    QColor mochaGray = QColor(127, 132, 156);      // 7f829c - Normal
    QColor mochaCode = QColor(180, 190, 200);      // Code blocks
    
    // Latte (light background) palette - muted pastel versions
    QColor latteImage = QColor(224, 128, 128);   // Soft coral rose - Images
    QColor latteBlue = QColor(130, 165, 215);      // Soft blue - H1, highlight
    QColor latteGreen = QColor(155, 195, 155);   // Soft sage green - H2
    QColor lattePeach = QColor(220, 165, 140);   // Soft peach - H3
    QColor latteTeal = QColor(145, 190, 185);      // Soft teal - Links
    QColor latteYellow = QColor(230, 200, 145);    // Soft yellow - Lists
    QColor latteGray = QColor(140, 143, 161);      // Soft gray - Normal
    QColor latteCode = QColor(160, 165, 180);      // Soft blue-gray - Code blocks
    
    // Select colors based on background brightness
    QColor colorImage = dark ? mochaImage : latteImage;
    QColor colorH1 = dark ? mochaBlue : latteBlue;
    QColor colorH2 = dark ? mochaGreen : latteGreen;
    QColor colorH3 = dark ? mochaPeach : lattePeach;
    QColor colorLink = dark ? mochaTeal : latteTeal;
    QColor colorList = dark ? mochaYellow : latteYellow;
    QColor colorNormal = dark ? mochaGray : latteGray;
    QColor colorCode = dark ? mochaCode : latteCode;
    QColor colorHighlight = dark ? mochaBlue : latteBlue;
    
    QTextDocument *doc = m_editor->document();
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    
    int blockCount = doc->blockCount();
    for (int i = 0; i < blockCount; ++i) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (!block.isValid())
            continue;
        
        QRectF blockRect = layout->blockBoundingRect(block);
        qreal y = blockRect.top() * m_scale;
        qreal h = qMax(2.0, blockRect.height() * m_scale);
        
        if (y > height())
            break;
        if (y + h < 0)
            continue;
        
        // Analyze block characteristics
        QColor color = colorNormal;
        
        // Get block format for list detection
        QTextBlockFormat blockFmt = block.blockFormat();
        
        // Check if block is part of a list
        QTextList *list = block.textList();
        bool isListItem = (list != nullptr);
        
        // Check for code block characteristics
        bool isCodeBlock = false;
        bool isPreformatted = (blockFmt.nonBreakableLines() || blockFmt.topMargin() > 5);
        
        QString text = block.text().trimmed();
        
        // Check character formats for code, links, and images
        bool hasCode = false;
        bool hasLink = false;
        bool hasImage = false;
        bool isBold = false;
        int maxFontSize = 0;
        int charCount = 0;
        
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;
            
            QTextCharFormat fmt = frag.charFormat();
            charCount += frag.length();
            
            // Check for images - only trust isImageFormat with a valid name
            if (fmt.isImageFormat()) {
                QTextImageFormat imgFmt = fmt.toImageFormat();
                if (!imgFmt.name().isEmpty()) {
                    hasImage = true;
                }
            }
            
            // Check font properties
            int fontSize = fmt.fontPointSize();
            if (fontSize > maxFontSize) maxFontSize = fontSize;
            if (fmt.fontWeight() >= QFont::Bold) isBold = true;
            
            // Check for code: monospace font or distinctive background
            QFont font = fmt.font();
            if (font.fixedPitch() || font.family().toLower().contains("mono")) {
                hasCode = true;
            }
            // Code often has a light gray background
            QColor bg = fmt.background().color();
            if (bg.isValid() && bg.lightness() > 0 && bg.lightness() < 240) {
                hasCode = true;
            }
            
            // Check for links using anchor or underline style
            if (fmt.isAnchor() || !fmt.anchorHref().isEmpty()) {
                hasLink = true;
            }
            // Links often have blue foreground and underline
            if (fmt.fontUnderline() && fmt.foreground().color().blue() > 150) {
                hasLink = true;
            }
        }
        
        // Determine block type based on collected information
        // (text was already captured above for table detection)
        
        // Code blocks: high percentage of code formatting or preformatted style
        if (isPreformatted && (hasCode || text.contains("  ") || text.startsWith("    "))) {
            isCodeBlock = true;
        }
        // Check for fenced code block markers in the text itself
        // (text is already trimmed from table detection above)
        if (text.startsWith("```") || text.startsWith("    ")) {
            isCodeBlock = true;
        }
        
        // Apply colors based on detection (priority: images > code > headings > links > lists > normal)
        // In plain text mode, skip fancy markdown detection (headings, links, lists)
        if (hasImage) {
            color = colorImage;
        } else if (isCodeBlock) {
            color = colorCode;
        } else if (!m_plainTextMode && (maxFontSize >= 14 || isBold)) {
            // Headings by font size or bold formatting (skip for plain text)
            if (maxFontSize >= 18 || (maxFontSize >= 14 && isBold)) {
                color = colorH1;
            } else if (maxFontSize >= 14 || (maxFontSize >= 12 && isBold)) {
                color = colorH2;
            } else {
                color = colorH3;
            }
        } else if (!m_plainTextMode && hasLink && charCount > 0) {
            color = colorLink;
        } else if (!m_plainTextMode && isListItem) {
            color = colorList;
        } else {
            color = colorNormal;
        }
        
        // Draw the block
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(color));
        painter.drawRect(QRectF(4, y, width() - 8, h));
    }
    
    // Draw viewport highlight
    QRectF viewportRect = getViewportRect();
    
    // Soft highlight matching theme
    painter.setPen(QPen(colorHighlight, 2));
    painter.setBrush(QColor(colorHighlight.red(), colorHighlight.green(), colorHighlight.blue(), 40));
    painter.drawRect(viewportRect.adjusted(0, 0, -1, -1));
}

void Minimap::mousePressEvent(QMouseEvent *event)
{
    if (!m_editor)
        return;
    
    m_dragging = true;
    qreal relativeY = static_cast<qreal>(event->pos().y()) / height();
    scrollEditorToPosition(relativeY);
}

void Minimap::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_editor || !m_dragging)
        return;
    
    qreal relativeY = static_cast<qreal>(event->pos().y()) / height();
    scrollEditorToPosition(relativeY);
}

void Minimap::wheelEvent(QWheelEvent *event)
{
    if (m_editor) {
        QCoreApplication::sendEvent(m_editor->viewport(), event);
    }
}

void Minimap::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    calculateScale();
}

void Minimap::onEditorScroll()
{
    update();
}

void Minimap::onEditorTextChanged()
{
    calculateScale();
    update();
}

void Minimap::scrollEditorToPosition(qreal relativeY)
{
    if (!m_editor || !m_editor->verticalScrollBar())
        return;
    
    QScrollBar *vScroll = m_editor->verticalScrollBar();
    qreal targetValue = relativeY * vScroll->maximum();
    
    targetValue = qMax(0.0, qMin(targetValue, static_cast<qreal>(vScroll->maximum())));
    
    vScroll->setValue(static_cast<int>(targetValue));
}
