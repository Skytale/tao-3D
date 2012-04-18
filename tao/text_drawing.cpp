// ****************************************************************************
//  text_drawing.cpp                                                Tao project
// ****************************************************************************
//
//   File Description:
//
//    Rendering of text
//
//
//
//
//
//
//
//
// ****************************************************************************
// This software is property of Taodyne SAS - Confidential
// Ce logiciel est la propriété de Taodyne SAS - Confidentiel
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "text_drawing.h"
#include "path3d.h"
#include "layout.h"
#include "widget.h"
#include "tao_utf8.h"
#include "drag.h"
#include "glyph_cache.h"
#include "gl_keepers.h"
#include "runtime.h"
#include "application.h"
#include "apply_changes.h"
#include "text_edit.h"
#include "tao_gl.h"
#include "tree-walk.h"

#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <sstream>

TAO_BEGIN

// ============================================================================
//
//   A text span is a contiguous string of characters with similar formatting
//
// ============================================================================

// Bitmap cache is enabled globally on platforms that do not support
// GL multisampling
bool TextUnit::cacheEnabled = false;

void TextUnit::Draw(Layout *where)
// ----------------------------------------------------------------------------
//   Render a portion of text and advance by the width of the text
// ----------------------------------------------------------------------------
{
    Widget     *widget     = where->Display();
    bool        hasLine    = setLineColor(where);
    bool        hasTexture = setTexture(where);
    GlyphCache &glyphs     = widget->glyphs();
    int         maxSize    = (int) glyphs.maxFontSize;
    bool        tooBig     = where->font.pointSize() > maxSize;
    bool        printing   = where->printing;
    Point3      offset0    = where->Offset();
    uint        dbgMod     = (Qt::ShiftModifier | Qt::ControlModifier);
    bool        dbgDirect  = (widget->lastModifiers() & dbgMod) == dbgMod;
    IFTRACE(justify)
    {
        std::cerr << "<->TextUnit::Draw(Layout *" << where<<") [" << this
                << "] offset0 :" << offset0 // << " font " << +font.toString()
                << " Layout font " << +where->font.toString()
                << " Layout color " << where->fillColor
                << std::endl;
        toDebugString(std::cerr);
    }

    if (!printing && !hasLine && !hasTexture && !tooBig && !dbgDirect &&
        cacheEnabled)
        DrawCached(where);
    else
        DrawDirect(where);

    IFTRACE(textselect)
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.8, 0.4, 0.3, 0.2);
        XL::Save<Point3> save(where->offset, offset0);
        Identify(where);
    }
}


void TextUnit::DrawCached(Layout *where)
// ----------------------------------------------------------------------------
//   Draw text span using cached textures
// ----------------------------------------------------------------------------
{
    Widget     *widget   = where->Display();
    GlyphCache &glyphs   = widget->glyphs();
    Point3      pos      = where->offset;
    Text *      ttree    = source;
    text        str      = ttree->value;
    bool        canSel   = ttree->Position() != XL::Tree::NOWHERE;
    TextSelect *sel      = widget->textSelection();
    QFont      &font     = where->font;
    uint64      texUnits = where->textureUnits;
    coord       x        = pos.x;
    coord       y        = pos.y;
    coord       z        = pos.z;

    GlyphCache::GlyphEntry  glyph;
    std::vector<Point3>     quads;
    std::vector<Point>      texCoords;


    if (canSel && (!where->id || IsMarkedConstant(ttree) ||
                   (sel && sel->textBoxId &&
                    ((TextFlow*)where)->textBoxIds.count(sel->textBoxId) == 0 )))
        canSel = false;

    // Compute per-char spread
    float spread = where->alongX.perSolid;

    // Loop over all characters in the text span
    uint i, max = str.length();
    for (i = start; i < max && i < end; i = XL::Utf8Next(str, i))
    {
        uint  unicode  = XL::Utf8Code(str, i);
        bool  newLine  = unicode == '\n';

        // Advance to next character
        if (newLine)
        {
            scale height = glyphs.Ascent(font, texUnits) + glyphs.Descent(font, texUnits);
            scale spacing = height + glyphs.Leading(font ,texUnits);
            x = 0;
            y -= spacing * glyphs.fontScaling;
        }
        else
        {
            // Find the glyph in the glyph cache
            if (!glyphs.Find(font, texUnits, unicode, glyph, false))
            {
                // Try to create the glyph
                if (!glyphs.Find(font, texUnits, unicode, glyph, true))
                    continue;
            }

            // Enter the geometry coordinates
            coord charX1 = x + glyph.bounds.lower.x;
            coord charX2 = x + glyph.bounds.upper.x;
            coord charY1 = y - glyph.bounds.lower.y;
            coord charY2 = y - glyph.bounds.upper.y;
            quads.push_back(Point3(charX1, charY1, z));
            quads.push_back(Point3(charX2, charY1, z));
            quads.push_back(Point3(charX2, charY2, z));
            quads.push_back(Point3(charX1, charY2, z));

            // Enter the texture coordinates
            Point &texL = glyph.texture.lower;
            Point &texU = glyph.texture.upper;
            texCoords.push_back(Point(texL.x, texL.y));
            texCoords.push_back(Point(texU.x, texL.y));
            texCoords.push_back(Point(texU.x, texU.y));
            texCoords.push_back(Point(texL.x, texU.y));

            x += glyph.advance + spread;
        }
    }

    // Check if there's anything to draw
    uint count = quads.size();
    if (count && setFillColor(where))
    {
        // Bind the glyph texture
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, glyphs.Texture());
        GLenum blur = GL_LINEAR;
        if (!where->hasPixelBlur &&
            font.pointSizeF() < glyphs.minFontSizeForAntialiasing)
            blur = GL_NEAREST;
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, blur);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, blur);
        glEnable(GL_TEXTURE_RECTANGLE_ARB);
        if (TaoApp->hasGLMultisample)
            glEnable(GL_MULTISAMPLE);

        // Assure that the last active texture unit is 0. Fix #1918.
        glClientActiveTexture(GL_TEXTURE0);
        // Draw a list of rectangles with the textures
        glVertexPointer(3, GL_DOUBLE, 0, &quads[0].x);
        glTexCoordPointer(2, GL_DOUBLE, 0, &texCoords[0].x);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glDrawArrays(GL_QUADS, 0, count);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    where->offset = Point3(x, y, z);
}


void TextUnit::DrawDirect(Layout *where)
// ----------------------------------------------------------------------------
//   Draw the given text directly using Qt paths
// ----------------------------------------------------------------------------
{
    Widget     *widget   = where->Display();
    GlyphCache &glyphs   = widget->glyphs();
    Point3      pos      = where->offset;
    Text *      ttree    = source;
    text        str      = ttree->value;
    bool        canSel   = ttree->Position() != XL::Tree::NOWHERE;
    TextSelect *sel      = widget->textSelection();
    QFont      &font     = where->font;
    uint64      texUnits = where->textureUnits;
    coord       x        = pos.x;
    coord       y        = pos.y;
    coord       z        = pos.z;
    scale       lw       = where->lineWidth;
    bool        skip     = false;
    uint        i, max   = str.length();
    TextFlow  * flow     = NULL;

    // Disable drawing of lines if we don't see them.
    if (where->lineColor.alpha <= 0)
        lw = 0;
    if ((flow = dynamic_cast<TextFlow*>(where)) && canSel)
    {
        if (!flow->currentTextBox->selectId || IsMarkedConstant(ttree) ||
            (sel && sel->textBoxId &&
             flow->textBoxIds.count(sel->textBoxId) == 0 ))
            canSel = false;
    }
    else
        canSel = false;

    GlyphCache::GlyphEntry  glyph;
    std::vector<Point3>     quads;
    std::vector<Point>      texCoords;

    // Find length of text span and compute per-char spread
    float spread = where->alongX.perSolid;

    // Loop over all characters in the text span
    for (i = start; i < max && i < end; i = XL::Utf8Next(str, i))
    {
        uint  unicode  = XL::Utf8Code(str, i);
        bool  newLine  = unicode == '\n';

        // Advance to next character
        if (newLine)
        {
            scale height = glyphs.Ascent(font, texUnits)
                         + glyphs.Descent(font, texUnits);
            scale spacing = height + glyphs.Leading(font, texUnits);
            x = 0;
            y -= spacing * glyphs.fontScaling;
        }
        else
        {
            // Find the glyph in the glyph cache
            if (!glyphs.Find(font, texUnits, unicode, glyph, true, true, lw))
                continue;

            GLMatrixKeeper save;
            glTranslatef(x, y, z);
            scale gscale = glyph.scalingFactor;
            glScalef(gscale, gscale, gscale);

            if (!skip)
            {
                setTexture(where);
                if (setFillColor(where))
                    glCallList(glyph.interior);
            }
            if (setLineColor(where))
                glCallList(glyph.outline);

            x += glyph.advance + spread;
        }
    }

    where->offset = Point3(x, y, z);
}


void TextUnit::DrawSelection(Layout *where)
// ----------------------------------------------------------------------------
//   Draw the selection for any selected character
// ----------------------------------------------------------------------------
//   Note that when drawing the rectangles, we don't include leading
//   (space between lines) in the rectangle being drawn.
//   For selection purpose we do include it to make it easier to click
{
    Widget     *widget       = where->Display();
    GlyphCache &glyphs       = widget->glyphs();
    Text *      ttree        = source;
    text        str          = ttree->value;
    bool        canSel       = ttree->Position() != XL::Tree::NOWHERE;
    QFont      &font         = where->font;
    uint64      texUnits     = where->textureUnits;
    Point3      pos          = where->offset;
    coord       x            = pos.x;
    coord       y            = pos.y;
    coord       z            = pos.z;
    scale       textWidth    = 0;
    TextSelect *sel          = widget->textSelection();
    uint        charId       = ~0U;
    scale       ascent       = glyphs.Ascent(font, texUnits);
    scale       descent      = glyphs.Descent(font, texUnits);
    scale       height       = ascent + descent;
    TextFlow   *flow         = NULL;
    GlyphCache::GlyphEntry  glyph;
    QString     selectedText;

    // A number of cases where we can't select text
    if ((flow = dynamic_cast<TextFlow*>(where)) && canSel)
    {
        if (!flow->currentTextBox->selectId || IsMarkedConstant(ttree) ||
            (sel && sel->textBoxId &&
             flow->textBoxIds.count(sel->textBoxId) == 0 ))
            canSel = false;
    }
    else
        canSel = false;

    // Find length of text span and compute per-char spread
    float spread = where->alongX.perSolid;

    // Loop over all characters in the text span
    uint i, next = 0, max = str.length();
    for (i = start; i < max && i < end; i = next)
    {
        uint unicode = XL::Utf8Code(str, i);
        next   = XL::Utf8Next(str, i);

        if (canSel)
            charId = where->CharacterId();

        // Fetch data about that glyph
        if (!glyphs.Find(font, texUnits, unicode, glyph, false))
            continue;

        if (sel && canSel)
        {
            // Mark characters in selection range
            sel->inSelection = charId>=sel->start() && charId<=sel->end();
            // Check up and down keys
            if (sel->inSelection || sel->needsPositions())
            {
                coord charX = x + glyph.bounds.lower.x;
                coord charY = y;

                sel->processChar(charId, charX, sel->inSelection, unicode);

                if (sel->inSelection)
                {
                    // Edit text in place if we have an editing request
                    if (sel->replace)
                    {
                        if (PerformEditOperation(widget, i) < 0)
                            next = i;
                        // Reset local copies after source->value changed
                        str = source->value;
                        max = str.length();
                    }
                    scale sd = glyph.scalingFactor * descent;
                    scale sh = glyph.scalingFactor * height;
                    sel->selBox |= Box3(charX,charY - sd,z, 1, sh, 0);

                    // Add the char to the selected text
                    if (charId != sel->end())
                        selectedText.append(QChar(unicode));

                    // Insert a tree from the clipboard if any
                    if (sel->replacement_tree && sel->point == charId)
                        PerformInsertOperation(where, widget, i);

                } // if(sel->inSelection)
            } // if (sel->inSelection || upDown)

            // Check if we are in a formula, if so display formula box
            if (sel->formulaMode)
            {
                coord charX = x + glyph.bounds.lower.x;
                coord charY = y;
                scale sd = glyph.scalingFactor * descent;
                scale sh = glyph.scalingFactor * height;
                sel->formulaBox |= Box3(charX,charY - sd,z, 1, sh, 0);
                sel->formulaMode--;
                if (!sel->formulaMode)
                {
                    glBlendFunc(GL_DST_COLOR, GL_ZERO);
                    text mode = "formula_highlight";
                    XL::Save<Point3> zeroOffset(where->offset, Point3());
                    widget->drawSelection(where, sel->formulaBox, mode, 0);
                    sel->formulaBox.Empty();
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
            }
        } // if(sel)

        // Advance to next character
        if (unicode == '\n')
        {
            scale spacing = height + glyphs.Leading(font,texUnits);
            x = 0;
            y -= spacing;
            textWidth = 0;
        }
        else
        {
            x += glyph.advance + spread;
            textWidth += glyph.advance + spread;
        }
    }

    // Update the selection data with the format and text
    // (to be used in copy/paste)
    if (sel && !selectedText.isEmpty())
    {
        QTextCursor &sc = sel->cursor;
        QTextBlockFormat bf = sc.blockFormat();
        if (modifyBlockFormat(bf, where) )
            sc.insertBlock(bf);
        QTextCharFormat cf = sc.charFormat();
        if (modifyCharFormat(cf, where) )
            sc.mergeCharFormat(cf);
        sc.insertText(selectedText);
    }

    if (sel && canSel)
    {
        if (max <= end)
        {
            charId++;
            if (charId >= sel->start() && charId <= sel->end())
            {
                if (sel->replace && sel->mark == sel->point)
                    PerformEditOperation(widget, i);
                scale sd = glyph.scalingFactor * descent;
                scale sh = glyph.scalingFactor * height;
                sel->selBox |= Box3(x,y - sd,z, 1, sh, 0);
            }
        }

        sel->last = charId;
    }

    where->offset = Point3(x, y, z);
}


void TextUnit::Identify(Layout *where)
// ----------------------------------------------------------------------------
//   Draw and identify the bounding boxes for the various characters
// ----------------------------------------------------------------------------
{
    Widget     *widget    = where->Display();
    GlyphCache &glyphs    = widget->glyphs();
    Text *      ttree     = source;
    text        str       = ttree->value;
    bool        canSel    = ttree->Position() != XL::Tree::NOWHERE;
    TextSelect *sel       = widget->textSelection();
    uint        charId    = ~0U;
    QFont      &font      = where->font;
    uint64      texUnits  = where->textureUnits;
    Point3      pos       = where->offset;
    coord       x         = pos.x;
    coord       y         = pos.y;
    coord       z         = pos.z;
    scale       textWidth = 0;
    scale       ascent    = glyphs.Ascent(font, texUnits);
    scale       descent   = glyphs.Descent(font, texUnits);
    scale       height    = ascent + descent;
    scale       sd        = descent ;
    scale       sh        = height;
    scale       charW     = 0;
    coord       charX1    = x;
    coord       charX2    = x;
    coord       charY1    = y;
    coord       charY2    = y;
    TextFlow   *flow      = NULL;

    GlyphCache::GlyphEntry  glyph;
    Point3                  quad[4];

    // A number of cases where we can't select text
    if ((flow = dynamic_cast<TextFlow*>(where)) && canSel)
    {
        if (!flow->currentTextBox->selectId || IsMarkedConstant(ttree) ||
            (sel && sel->textBoxId &&
             flow->textBoxIds.count(sel->textBoxId) == 0 ))
            canSel = false;
    }
    else
        canSel = false;

    // Find length of text span and compute per-char spread
    float spread = where->alongX.perSolid;

    // Prepare to draw with the quad
    glVertexPointer(3, GL_DOUBLE, 0, &quad[0].x);
    glEnableClientState(GL_VERTEX_ARRAY);

    // Loop over all characters in the text span
    uint i, next = 0, max = str.length();
    for (i = start; i < max && i < end; i = next)
    {
        uint unicode = XL::Utf8Code(str, i);
        next   = XL::Utf8Next(str, i);

        if (canSel)
            charId = where->CharacterId();

        // Fetch data about that glyph
        if (!glyphs.Find(font, texUnits, unicode, glyph, false))
            continue;

        if (canSel)
            glLoadName(charId | Widget::CHARACTER_SELECTED);

        sd = glyph.scalingFactor * descent;
        sh = glyph.scalingFactor * height;
        charW = glyph.bounds.Width() / 2;
        charX2 = x + glyph.bounds.upper.x - charW/2;
        charY1 = y - sd;
        charY2 = y - sd + sh;

        quad[0] = Point3(charX1, charY1, z);
        quad[1] = Point3(charX2, charY1, z);
        quad[2] = Point3(charX2, charY2, z);
        quad[3] = Point3(charX1, charY2, z);
        glDrawArrays(GL_QUADS, 0, 4);

        // Advance to next character
        if (unicode == '\n')
        {
            scale spacing = height + glyphs.Leading(font, texUnits);
            x = 0;
            y -= spacing;
            textWidth = 0;
            charX1 = x;
        }
        else
        {
            charX1 = charX2;
            x += glyph.advance + spread;
            textWidth += glyph.advance + spread;
        }
    }

    // Draw a trailing block
    if (sel && canSel && max <= end)
    {
        charId++;
        glLoadName(charId | Widget::CHARACTER_SELECTED);
        if (glyphs.Find(font, ' ', texUnits, glyph, false))
        {
            sd = glyph.scalingFactor * descent;
            sh = glyph.scalingFactor * height;
            charW = glyph.bounds.Width() / 2;
            charX2 = x + glyph.bounds.upper.x - charW/2;
            charY1 = y - sd;
            charY2 = y - sd + sh;

            quad[0] = Point3(charX1, charY1, z);
            quad[1] = Point3(charX2, charY1, z);
            quad[2] = Point3(charX2, charY2, z);
            quad[3] = Point3(charX1, charY2, z);
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }

    // Disable drawing with the quad
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    where->offset = Point3(x, y, z);
}


void TextUnit::Draw(GraphicPath &path, Layout *l)
// ----------------------------------------------------------------------------
//   Render a portion of text and advance by the width of the text
// ----------------------------------------------------------------------------
{
    IFTRACE(justify)
    {
        std::cerr << "->TextUnit::Draw (GraphicPath &path, Layout *"
                << l << ") [" << this << "] layout font"
                << +l->font.toString() << "\n";
        toDebugString(std::cerr);
    }
    Point3 position = path.position;
    QFontMetricsF fm(l->font);
    QPainterPath qt;

    QString str = +source->value.substr(start, end - start);
    int index = str.indexOf(QChar('\n'));
    while (index >= 0)
    {
        QString fragment = str.left(index);
        qt.addText(position.x, -position.y, l->font, fragment);
        position.x = 0;
        position.y -= fm.height();
        str = str.mid(index+1);
        index = str.indexOf(QChar('\n'));
    }

    qt.addText(position.x, -position.y, l->font, str);
    position.x += fm.width(str);

    path.addQtPath(qt, -1);
    path.moveTo(position);
    IFTRACE(justify)
    {
        std::cerr << "<-TextUnit::Draw (GraphicPath &path, Layout *"
                << l << ") [" << this << "]\n";
    }
}


Box3 TextUnit::Bounds(Layout *where)
// ----------------------------------------------------------------------------
//   Return the smallest box that surrounds the text
// ----------------------------------------------------------------------------
{
    Widget     *widget   = where->Display();
    GlyphCache &glyphs   = widget->glyphs();
    text        str      = source->value;
    QFont      &font     = where->font;
    uint64      texUnits = where->textureUnits;
    Box3        result;
    scale       ascent   = glyphs.Ascent(font, texUnits);
    scale       descent  = glyphs.Descent(font, texUnits);
    scale       leading  = glyphs.Leading(font, texUnits);
    Point3      pos      = where->offset;
    coord       x        = pos.x;
    coord       y        = pos.y;
    coord       z        = pos.z;

    GlyphCache::GlyphEntry  glyph;

    // Loop over all characters in the text span
    uint i, max = str.length();
    for (i = start; i < max && i < end; i = XL::Utf8Next(str, i))
    {
        uint  unicode  = XL::Utf8Code(str, i);
        bool  newLine  = unicode == '\n';

        // Find the glyph in the glyph cache
        if (!glyphs.Find(font, texUnits, unicode, glyph, true))
            continue;

        scale sa = ascent * glyph.scalingFactor;
        scale sd = descent * glyph.scalingFactor;
        scale sl = leading * glyph.scalingFactor;

        // Enter the geometry coordinates
        coord charX1 = x + glyph.bounds.lower.x;
        coord charX2 = x + glyph.bounds.upper.x;
        coord charY1 = y - glyph.bounds.lower.y;
        coord charY2 = y - glyph.bounds.upper.y;

        // Advance to next character
        if (newLine)
        {
            result |= Point3(charX1, y + sa, z);
            result |= Point3(charX1, y - sd - sl, z);

            scale height = glyphs.Ascent(font, texUnits) +
                    glyphs.Descent(font, texUnits);
            scale spacing = height + glyphs.Leading(font, texUnits);
            x = 0;
            y -= spacing * glyph.scalingFactor;
        }
        else
        {
            result |= Point3(charX1, charY1, z);
            result |= Point3(charX2, charY2, z);

            x += glyph.advance;
        }
    }
    where->offset = Point3(x,y,z);

    return result;
}


Box3 TextUnit::Space(Layout *where)
// ----------------------------------------------------------------------------
//   Return the box that surrounds the text, including leading
// ----------------------------------------------------------------------------
{
    Widget     *widget   = where->Display();
    GlyphCache &glyphs   = widget->glyphs();
    text        str      = source->value;
    QFont      &font     = where->font;
    uint64      texUnits = where->textureUnits;
    Box3        result;
    scale       ascent   = glyphs.Ascent(font, texUnits);
    scale       descent  = glyphs.Descent(font, texUnits);
    scale       leading  = glyphs.Leading(font, texUnits);
    Point3      pos      = where->offset;
    coord       x        = pos.x;
    coord       y        = pos.y;
    coord       z        = pos.z;

    GlyphCache::GlyphEntry  glyph;
    IFTRACE(justify)
    {
        std::cerr << "->TextUnit::Space(Layout *" << where<< ") offset "
                << where->Offset() << " for ";
        toDebugString(std::cerr);
    }

    // Loop over all characters in the text span
    uint i, max = str.length();
    for (i = start; i < max && i < end; i = XL::Utf8Next(str, i))
    {
        uint  unicode  = XL::Utf8Code(str, i);
        bool  newLine  = unicode == '\n';

        // Find the glyph in the glyph cache
        if (!glyphs.Find(font, texUnits, unicode, glyph, true))
            continue;

        scale sa = ascent * glyph.scalingFactor;
        scale sd = descent * glyph.scalingFactor;
        scale sl = leading * glyph.scalingFactor;

        // Enter the geometry coordinates
        coord charX1 = x + glyph.bounds.lower.x;
        coord charX2 = x + glyph.bounds.upper.x;
        coord charY1 = y - glyph.bounds.lower.y;
        coord charY2 = y - glyph.bounds.upper.y;

        result |= Point3(charX1, charY1, z);
        result |= Point3(charX2, charY2, z);
        result |= Point3(charX1, y + sa, z);

        // Advance to next character
        if (newLine)
        {
            IFTRACE(justify)
            {
                std::cerr << "--TextUnit::Space(Layout *" << where<< ")  newline "
                          << " y (" << y << ") -= sa + sd + sl ("<< sa + sd + sl
                          << ") ==> " << y - (sa + sd + sl) << std::endl;
            }

            result |= Point3(charX1, y - sd - sl, z);
            x = 0;
            y -= sa + sd + sl;
        }
        else
        {
            result |= Point3(charX1 + glyph.advance, y - sd - sl, z);
            x += glyph.advance;
        }
    }
    where->offset = Point3(x,y,z);
    IFTRACE(justify)
    {
        std::cerr << "<-TextUnit::Space(Layout *" << where<< ") offset "
                  << where->Offset() << " result "
                  << result << "\n";
        //toDebugString(std::cerr);
    }
    return result;
}


TextUnit *TextUnit::Break(BreakOrder &order, uint &size)
// ----------------------------------------------------------------------------
//   If the text span contains a word or line break, cut there
// ----------------------------------------------------------------------------
{
    text str = source->value;
    uint i, max = str.length();
    for (i = start; i < max && i < end; i = XL::Utf8Next(str, i))
    {
        QChar c = QChar(XL::Utf8Code(str, i));
        BreakOrder charOrder = CharBreak;
        size++;
        if (c.isSpace())
        {
            charOrder = WordBreak;
            if (c == '\n')
                charOrder = ParaBreak;
            else if (c == '\r')
                charOrder = LineBreak;
            else if (c == '\f')
                charOrder = SentenceBreak;
        }

        if (order <= charOrder)
        {
            // Create two text spans, the first one containing the split
            uint next = XL::Utf8Next(str, i);
            TextUnit *result = NULL;
            if (next < max && next < end)
                result = new TextUnit(source, next, end);

            order = charOrder;
            end = next;
            return result;
        }
    }
    order = NoBreak;
    return NULL;
}


void TextUnit::toDebugString(std::ostream &out)
// ----------------------------------------------------------------------------
//   Print the Text unit value on the given ostream
// ----------------------------------------------------------------------------
{
    out << "TextUnit\n" << source->value <<std::endl;
    uint loc_end = end;
    if (loc_end == (uint)~0)
    {
        loc_end = source->value.size();
    }
    if (source->value.size() == 0)
    {
        out << "EMPTY\n";
        return;
    }
    for (uint i = 0; i< start; i++)
    {
        if (XL::Utf8Code(source->value, i) == '\n')
            out <<"\n";
        else
            out << " ";
    }
    out << "^";
    if (start == loc_end || loc_end == 0)
    {
        out << "\n";
        return;
    }

    for (uint i = start+1; i< loc_end-1 ; i++)
    {
        if (XL::Utf8Code(source->value, i) == '\n')
            out <<"\n";
        else
            out << "-";
    }
    out << "^\n";
}


void TextUnit::toText(std::ostream &out)
// ----------------------------------------------------------------------------
//   Print the Text unit value on the given ostream
// ----------------------------------------------------------------------------
{
    uint loc_end = end;
    if (loc_end == (uint)~0)
    {
        loc_end = source->value.size();
    }
    if (source->value.size() == 0 || start == loc_end || loc_end == 0)
    {
        out << "EMPTY\n";
        return;
    }

    out << "|";
    for (uint i = start; i< loc_end ; i++)
        out << (char)XL::Utf8Code(source->value, i);

    out << "|\n";
}


scale TextUnit::TrailingSpaceSize(Layout *where)
// ----------------------------------------------------------------------------
//   Return the size of all the spaces at the end of the value
// ----------------------------------------------------------------------------
{
    Widget     *widget   = where->Display();
    GlyphCache &glyphs   = widget->glyphs();
    QFont      &font     = where->font;
    uint64      texUnits = where->textureUnits;
    text        str      = source->value;
    uint        pos      = str.length();
    Box3        box;

    if (pos > end)
        pos = end;
    while (pos > start)
    {
        pos = XL::Utf8Previous(str, pos);
        QChar c = QChar(XL::Utf8Code(str, pos));
        if (!c.isSpace())
            break;

        // Find the glyph in the glyph cache
        GlyphCache::GlyphEntry  glyph;
        uint  unicode  = XL::Utf8Code(str, pos);
        if (!glyphs.Find(font, texUnits, unicode, glyph, true))
            continue;
        // Enter the geometry coordinates
        coord charX1 = glyph.bounds.lower.x;
        coord charX2 = glyph.bounds.upper.x;
        coord charY1 = glyph.bounds.lower.y;
        coord charY2 = glyph.bounds.upper.y;
        box |= Point3(charX1, charY1, 0);
        box |= Point3(charX2, charY2, 0);
        box |= Point3(charX1 + glyph.advance, charY1, 0);
    }

    scale result = box.Width();
    if (result < 0)
        result = 0;
    IFTRACE(justify)
    {
        std::cerr << "<->TextUnit::TrailingSpaceSize[" << this << "] font "
                << +where->font.toString() <<" returns " << result<< " for ";
        toText(std::cerr);
    }
   return result;
}


int TextUnit::PerformEditOperation(Widget *widget, uint i)
// ----------------------------------------------------------------------------
//   Perform text editing operations (insert, replace, ...)
// ----------------------------------------------------------------------------
//   Returns the difference of source's length between entry and exit time.
{
    TextSelect *sel           = widget->textSelection();
    text        rpl           = sel->replacement;
    uint        length        = XL::Utf8Length(rpl);
    kstring     commitMessage = "";
    uint        eos           = i;
    text        str           = source->value;
    uint        entryLen      = str.length();
    if (!sel->length() && !length)
    {
        sel->replace = false;
        return 0; // Nothing to do
    }

    if (length && sel->length())
        commitMessage = "Replaced text";
    else if (length && !sel->length())
        commitMessage = "Inserted text";
    else if (sel->length() && !length)
        commitMessage = "Deleted text";

    // Mark the changes with commit message, abort if source window changed
    if (!widget->markChange(commitMessage))
        return 0;

    // Compute the end of selection Id based on the selection length
    // and move the cursor accordingly
    uint tmp = eos;
    uint originalSelectionLength = sel->length();
    for (uint l = 0; l < originalSelectionLength; l++)
    {
        tmp = XL::Utf8Next(str, eos);
        if (tmp == eos)
        {
            // End of current text span is reached,
            // the selection continues on next text span.
            break;
        }
        if (sel->mark > sel->point)
            sel->point++;
        else
            sel->mark++;
        eos = tmp;
    }

    // Replace the text
    source->value.replace(i, eos-i, rpl);

    // Reset replacement data
    sel->replacement = "";
    int deltaSelection = length - (originalSelectionLength - sel->length());

    // Edition completed when selection mark and point are equal
    if (sel->mark == sel->point)
    {
        // Move the cursor
        sel->moveTo(sel->mark + deltaSelection);

        // Reset replacement data
        sel->replace = false;
    }
    else
    {
        sel->mark  += deltaSelection;
        sel->point += deltaSelection;
    }
    uint exitLen = source->value.length();

    // Reload the program
    widget->updateProgramSource();

    return exitLen - entryLen;
}


void TextUnit::PerformInsertOperation(Layout * /* l */,
                                      Widget * widget,
                                      uint     position)
// ----------------------------------------------------------------------------
//   Insert replacement_tree of textSelection at the current cursor location.
// ----------------------------------------------------------------------------
{
    TextSelect *sel = widget->textSelection();
    if (sel->replacement_tree &&
        widget->markChange("Clipboard content pasted"))
    {
        TreeList list;
        // Cut the span at the current position
        text endOfSpan  = source->value.substr(position);
        text headOfSpan = source->value.substr(0, position);
        // Begining of previous text
        if (headOfSpan.size())
        {
            XL::Prefix *headOfText= new XL::Prefix(new XL::Name("text"),
                                        new XL::Text(headOfSpan,
                                                     source->opening,
                                                     source->closing));
            list.push_back(headOfText);
        }

        // Text to insert
        XL::Prefix * insertedTextSpan =
                new XL::Prefix(new XL::Name("text_span"),
                               new XL::Block(sel->replacement_tree->AsInfix(),
                                             XL::Block::indent,
                                             XL::Block::unindent));
        list.push_back(insertedTextSpan);

        // End of previous text
        if (endOfSpan.size())
        {
            XL::Prefix *endOfText = new XL::Prefix(new XL::Name("text"),
                                       new XL::Text(endOfSpan,
                                                    source->opening,
                                                    source->closing));
            list.push_back(endOfText);
        }

        sel->replacement_tree = NULL;

        // Insert the resulting tree in place of the parent's source:
        // Source is the text surounded by "" or <<>> itself,
        // its parent is the prefix text, so search for the grand parent.
        XL::FindParentAction getGrandParent(source, 2);
        Tree * grandParent = widget->xlProgram->tree->Do(getGrandParent);
        if (!getGrandParent.path.size())
            return;
        char lastChar = getGrandParent.path.at(getGrandParent.path.size() - 1);

        XL::Infix   *inf  = grandParent->AsInfix();
        if (inf)
        {
            if (lastChar == 'l')
            {
                list.push_back(inf->right);
                XL::Infix * head = (XL::Infix*)xl_list_to_tree(list, "\n");

                inf->left = head->left;
                inf->right = head->right;
            }
            else
            {
                XL::Infix * head = (XL::Infix*)xl_list_to_tree(list, "\n");
                inf->right = head;
            }
        }
        else
        {
            XL::Infix   *head = (XL::Infix*)xl_list_to_tree(list, "\n");

            XL::Block   *bl   = grandParent->AsBlock();
            XL::Prefix  *pre  = grandParent->AsPrefix();
            XL::Postfix *post = grandParent->AsPostfix();

            if (bl)
            {
                bl->child = head;
            }
            else if (pre)
            {
                if (lastChar == 'l')
                    pre->left = head;
                else
                    pre->right = head;
            }
            else if (post)
            {
                if (lastChar == 'l')
                    post->left = head;
                else
                    post->right = head;
            }
        }
        sel->moveTo(sel->start());

        // Reload the program
        widget->reloadProgram();
        widget->runOnNextDraw = true;

    }
}



// ============================================================================
//
//    A text formula is used to display numerical / evaluated values
//
// ============================================================================

uint TextFormula::formulas = 0;
uint TextFormula::shows = 0;

XL::Text *TextFormula::Format(XL::Prefix *self)
// ----------------------------------------------------------------------------
//   Return a formatted value for the given value
// ----------------------------------------------------------------------------
{
    Tree *value = self->right;
    TextFormulaEditInfo *info = value->GetInfo<TextFormulaEditInfo>();
    formulas++;
    shows = 0;

    // Check if this formula is currently being edited, if so return its text
    if (info && info->order == formulas)
        return info->source;

    // If the value is a name, we evaluate it in its normal symbol table
    Name *name = value->AsName();
    Context *context = widget->formulasContext();
    if (name)
        if (Tree *named = context->Bound(name))
            value = named;

    // Evaluate the tree and turn it into a tree
    Tree *computed = context->Evaluate(value);
    return new XL::Text(*computed);
}


void TextFormula::DrawSelection(Layout *where)
// ----------------------------------------------------------------------------
//   Detect if we edit a formula, if so create its FormulEditInfo
// ----------------------------------------------------------------------------
{
    Widget              *widget = where->Display();
    TextSelect          *sel    = widget->textSelection();
    XL::Prefix *         prefix = self;
    XL::Tree *           value  = prefix->right;
    TextFormulaEditInfo *info   = value->GetInfo<TextFormulaEditInfo>();
    uint                 charId = where->CharacterId();

    // Count formulas to identify them uniquely
    shows++;
    formulas = 0;

    // Check if formula is selected and we are not editing it
    if (sel && sel->textMode)
    {
        if (!info && widget->selected(charId | Widget::CHARACTER_SELECTED))
        {
            // No info: create one

            // If the value is a name, we evaluate it in its normal symbol table
            Name *name = value->AsName();
            Context *context = widget->formulasContext();
            if (name)
                if (Tree *named = context->Bound(name))
                    value = named;


            text edited = text("  ") + text(*value) + "  ";
            Text *editor = new Text(edited, "\"", "\"", value->Position());
            info = new TextFormulaEditInfo(editor, shows);
            prefix->right->SetInfo<TextFormulaEditInfo>(info);

            // Update mark and point
            Text *source = info->source;
            uint length = source->value.length();
            sel->point = charId;
            sel->mark = charId + length;

            widget->updateGL();
        }
    }

    // Indicate how many characters we want to display as "formula"
    if (info && sel)
    {
        if (shows == info->order)
        {
            Text *source = info->source;
            sel->formulaMode = source->value.length() + 1;
        }
        else
        {
            Text *source = this->source;
            sel->formulaMode = source->value.length() + 1;
        }
    }

    TextUnit::DrawSelection(where);

    // Check if the cursor moves out of the selection - If so, validate
    if (info &&info->order == shows)
    {
        XL::Text *source = info->source;
        uint length = source->value.length();

        if (!sel || (sel->mark == sel->point &&
                     (sel->point < charId || sel->point > charId + length)))
        {
            if (Validate(info->source, widget))
            {
                if (sel && sel->point > charId + length)
                {
                    sel->point -= length;
                    sel->mark -= length;
                    sel->updateSelection();
                }
            }
        }
    }
    else if (!info && sel && charId >= sel->start() && charId <= sel->end())
    {
        // First run, make sure we return here to create the editor
        widget->updateGL();
    }
}


void TextFormula::Identify(Layout *where)
// ----------------------------------------------------------------------------
//   Give one ID to the whole formula so that we can click on it
// ----------------------------------------------------------------------------
{
    XL::Prefix *         prefix = self->AsPrefix();
    XL::Tree *           value  = prefix->right;
    TextFormulaEditInfo *info   = value->GetInfo<TextFormulaEditInfo>();
    uint                 charId = where->CharacterId();
    Widget              *widget = where->Display();
    TextSelect          *sel    = widget->textSelection();

    if (!info && where->id)
        glLoadName(charId | Widget::CHARACTER_SELECTED);

    if (sel)
        sel->last = charId + 1;

    TextUnit::Identify(where);
}


bool TextFormula::Validate(Text *source, Widget *widget)
// ----------------------------------------------------------------------------
//   Check if we can parse the input. If so, update self
// ----------------------------------------------------------------------------
{
    std::istringstream  input(source->value);
    XL::Syntax          syntax (XL::MAIN->syntax);
    XL::Positions      &positions = XL::MAIN->positions;
    XL::Errors          errors;
    XL::Parser          parser(input, syntax,positions,errors);
    Tree *              newTree = parser.Parse();

    if (newTree && widget->markChange("Replaced formula"))
    {
        Prefix *prefix = self;
        prefix->right = newTree;
        widget->reloadProgram();
        return true;
    }
    return false;
}



// ============================================================================
//
//    A text value is used to display numerical / evaluated values
//
// ============================================================================

uint TextValue::values = 0;
uint TextValue::shows = 0;

XL::Text *TextValue::Format(XL::Tree *value)
// ----------------------------------------------------------------------------
//   Return a formatted value for the given value
// ----------------------------------------------------------------------------
{
    TextFormulaEditInfo *info = value->GetInfo<TextFormulaEditInfo>();
    values++;
    shows = 0;

    // Check if this value is currently being edited, if so return its text
    if (info && info->order == values)
        return info->source;

    // Evaluate the tree and turn it into a tree
    return new XL::Text(*value);
}


void TextValue::DrawSelection(Layout *where)
// ----------------------------------------------------------------------------
//   Detect if we edit a value, if so create its FormulEditInfo
// ----------------------------------------------------------------------------
{
    Widget              *widget = where->Display();
    TextSelect          *sel    = widget->textSelection();
    TextFormulaEditInfo *info   = value->GetInfo<TextFormulaEditInfo>();
    uint                 charId = where->CharacterId();

    // Count values to identify them uniquely
    shows++;
    values = 0;

    // Check if value is selected and we are not editing it
    if (sel && sel->textMode)
    {
        if (!info && widget->selected(charId | Widget::CHARACTER_SELECTED))
        {
            // No info: create one
            text edited = text("   ") + text(*value) + "  ";
            Text *editor = new Text(edited, "\"", "\"", value->Position());
            info = new TextFormulaEditInfo(editor, shows);
            value->SetInfo<TextFormulaEditInfo>(info);

            // Update mark and point
            XL::Text *source = info->source;
            uint length = source->value.length();
            sel->point = charId;
            sel->mark = charId + length;

            widget->updateGL();
        }
    }

    // Indicate how many characters we want to display as "value"
    if (info && sel)
    {
        if (shows == info->order)
        {
            XL::Text *source = info->source;
            sel->formulaMode = source->value.length() + 1;
        }
        else
        {
            XL::Text *source = this->source;
            sel->formulaMode = source->value.length() + 1;
        }
    }

    TextUnit::DrawSelection(where);

    // Check if the cursor moves out of the selection - If so, validate
    if (info &&info->order == shows)
    {
        XL::Text *source = info->source;
        uint length = source->value.length();

        if (!sel || (sel->mark == sel->point &&
                     (sel->point < charId || sel->point > charId + length)))
        {
            if (Validate(info->source, widget))
            {
                if (sel && sel->point > charId + length)
                {
                    sel->point -= length;
                    sel->mark -= length;
                    sel->updateSelection();
                }
            }
        }
    }
    else if (!info && sel && charId >= sel->start() && charId <= sel->end())
    {
        // First run, make sure we return here to create the editor
        widget->refreshNow();
    }
}


void TextValue::Identify(Layout *where)
// ----------------------------------------------------------------------------
//   Give one ID to the whole value so that we can click on it
// ----------------------------------------------------------------------------
{
    TextFormulaEditInfo *info   = value->GetInfo<TextFormulaEditInfo>();
    uint                 charId = where->CharacterId();
    Widget              *widget = where->Display();
    TextSelect          *sel    = widget->textSelection();

    if (!info && where->id)
        glLoadName(charId | Widget::CHARACTER_SELECTED);

    if (sel)
        sel->last = charId + 1;

    TextUnit::Identify(where);
}


bool TextValue::Validate(XL::Text *source, Widget *widget)
// ----------------------------------------------------------------------------
//   Check if we can parse the input. If so, update self
// ----------------------------------------------------------------------------
{
    std::istringstream  input(source->value);
    XL::Syntax          syntax (XL::MAIN->syntax);
    XL::Positions      &positions = XL::MAIN->positions;
    XL::Errors          errors;
    XL::Parser          parser(input, syntax,positions,errors);
    Tree *              newTree   = parser.Parse();
    bool                valid = false;

    if (newTree && widget->markChange("Replaced value"))
    {
        if (Real *oldReal = value->AsReal())
        {
            valid = true;
            if (Real *newReal = newTree->AsReal())
                oldReal->value = newReal->value;
            else if (Integer *newInteger = newTree->AsInteger())
                oldReal->value = newInteger->value;
            else
                valid = false;
        }
        else if (Integer *oldInteger = value->AsInteger())
        {
            valid = true;
            if (Integer *newInteger = newTree->AsInteger())
                oldInteger->value = newInteger->value;
            else
                valid = false;
        }
        else if (Name *oldName= value->AsName())
        {
            valid = true;
            if (Name *newName = newTree->AsName())
                oldName->value = newName->value;
            else
                valid = false;
        }
        else if (Text *oldText= value->AsText())
        {
            valid = true;
            if (Text *newText = newTree->AsText())
                oldText->value = newText->value;
            else
                valid = false;
        }

        if (valid)
            widget->reloadProgram();
        return valid;
    }
    return false;
}



// ============================================================================
//
//   A text selection identifies a range of text being edited
//
// ============================================================================

TextSelect::TextSelect(Widget *w)
// ----------------------------------------------------------------------------
//   Constructor initializes an empty text range
// ----------------------------------------------------------------------------
    : Identify("Text selection", w),
      mark(0), point(0), previous(0), last(0), textBoxId(0),
      direction(None), targetX(0),
      replacement(""), replace(false),
      textMode(false),
      pickingLineEnds(false), pickingUpDown(false), movePointOnly(false),
      formulaMode(false),cursor(QTextCursor(new QTextDocument(""))),
      replacement_tree(NULL), inSelection(false)
{
    Widget::selection_map::iterator i, last = w->selection.end();
    for (i = w->selection.begin(); i != last; i++)
    {
        uint id = (*i).first;
        if (id & Widget::CHARACTER_SELECTED)
        {
            id &= Widget::CHARACTER_SELECTED;
            if (!mark)
                mark = point = id;
            else
                point = id;
        }
    }
}


Activity *TextSelect::Display()
// ----------------------------------------------------------------------------
//   Display the text selection
// ----------------------------------------------------------------------------
{
    return next;
}


Activity *TextSelect::Idle()
// ----------------------------------------------------------------------------
//   Idle activity
// ----------------------------------------------------------------------------
{
    return next;
}


Activity *TextSelect::Key(text key)
// ----------------------------------------------------------------------------
//    Tell that we want keys to be passed down to the document
// ----------------------------------------------------------------------------
{
    (void) key;
    return next;
}


Activity *TextSelect::Edit(text key)
// ----------------------------------------------------------------------------
//   Perform text editing activities
// ----------------------------------------------------------------------------
{
    if (!textMode)
        return next;

    if (key == "Space")                 key = " ";
    else if (key == "Shift-Space")      key = " ";
    else if (key == "Return")           key = "\n";
    else if (key == "Enter")            key = "\n";

    if (key == "Left")
    {
        uint pos = start();
        if (pos > 1 && !hasSelection())
            pos--;
        moveTo(pos);
        direction = Left;
    }
    else if (key == "Right")
    {
        uint pos = end();
        if (pos < last && !hasSelection())
            pos++;
        moveTo(pos);
        direction = Right;
    }
    else if (key == "Shift-Left")
    {
        point--;
        direction = Left;
    }
    else if (key == "Shift-Right")
    {
        point++;
        direction = Right;
    }
    else if (key == "Up")
    {
        mark = point = start();
        direction = Up;
        movePointOnly = false;
    }
    else if (key == "Down")
    {
        mark = point = end();
        direction = Down;
        movePointOnly = false;
    }
    else if (key == "Shift-Up")
    {
        direction = Up;
        movePointOnly = true;
    }
    else if (key == "Shift-Down")
    {
        direction = Down;
        movePointOnly = true;
    }
    else if (key == "Delete" || key == "Backspace")
    {
        replacement = "";
        replace = true;
        if (!hasSelection())
            point = (key == "Delete") ? point+1 : point-1;
        direction = Mark;
    }
    else if (XL::Utf8Length(key) == 1)
    {
        replacement = key;
        replace = true;
        direction = Mark;
    }
    else
    {
        // We don't know what to do with that key
        return next;
    }

    if (replace)
    {
        widget->updateSelection();
        mark = point;
        replace = false;
    }

    updateSelection();

    // We have handled this activity
    return NULL;
}


Activity *TextSelect::Click(uint button, uint count, int x, int y)
// ----------------------------------------------------------------------------
//   Selection of text
// ----------------------------------------------------------------------------
{
    if (button & Qt::LeftButton)
    {
        if (count)
        {
            mark = point = 0;
            if (count == 2)
                textMode = true;
            direction = Mark;
            return MouseMove(x, y, true);
        }
    }
    return next;
}


Activity *TextSelect::MouseMove(int x, int y, bool active)
// ----------------------------------------------------------------------------
//   Move text selection
// ----------------------------------------------------------------------------
{
    if (!active)
        return next;

    y = widget->height() - y;
    Box rectangle(x, y, 1, 1);

    // Get the object at the click point
    GLuint selected      = 0;
    GLuint handleId      = 0;
    GLuint charSelected  = 0;
    GLuint childSelected = 0;
    selected = ObjectInRectangle(rectangle,
                                 &handleId, &charSelected, &childSelected);
    if (selected)
    {
        if (handleId)
        {
            textMode = false;
        }
        else if (charSelected)
        {
            if (textMode)
            {
                if (mark)
                    point = charSelected;
                else
                    mark = point = charSelected;
                textBoxId = selected;
                updateSelection();
            }
            else if (!mark)
            {
                mark = point = selected;
            }
        }
    }

    // If selecting a range of text, prevent the drag from moving us around
    // BUG #891 This is to late

    // Need a refresh
    widget->updateGL();

    // Let a possible selection do its own stuff
    return next;
}


void TextSelect::updateSelection()
// ----------------------------------------------------------------------------
//   Update the selection of the widget based on mark and point
// ----------------------------------------------------------------------------
{
    widget->selection.clear();
    uint s = start(), e = end(), marker = Widget::CHARACTER_SELECTED;
    for (uint i = s; i <= e; i++)
        widget->select(i | marker, marker);

    if (textBoxId)
    {
        widget->select(textBoxId, Widget::CONTAINER_OPENED);
        widget->refreshOn(QEvent::KeyPress);
        widget->refreshOn(QEvent::KeyRelease);
    }
    formulaMode = false;
    widget->updateGL();
}


void TextSelect::processLineBreak()
// ----------------------------------------------------------------------------
//   Mark the beginning of a new drawing line for Up/Down keys
// ----------------------------------------------------------------------------
{
    pickingLineEnds = true;
}


void TextSelect::processChar(uint charId, coord x, bool selected, uint code)
// ----------------------------------------------------------------------------
//   Record a new character and deal with Up/Down keys
// ----------------------------------------------------------------------------
{
    bool up = direction == Up;
    bool down = direction == Down;
    bool lineBreak = code == '\n';
    if (!up && !down)
    {
        if (selected && direction != None)
            targetX = x;
        return;
    }

    // Check if we are at beginning or end of line, if so find ends
    if (pickingLineEnds)
    {
        // Next character we will look at is not a line boundary
        pickingLineEnds = false;

        if (down)
        {
            // Current best position
            if (charId > point)
            {
                if (pickingUpDown || x >= targetX || lineBreak)
                {
                    // What we had was the best position
                    point = charId;
                    if (!movePointOnly)
                        mark = point;
                    direction = None;
                    pickingUpDown = false;
                    updateSelection();
                }
                else
                {
                    // Best default position is start of line
                    pickingUpDown = true;
                }
            }
            else
            {
                pickingUpDown = false;
            }
        }
        else // Up
        {
            if (charId < point)
            {
                if (pickingUpDown || x >= targetX || lineBreak)
                    previous = charId; // We are at right of previous line
                else
                    pickingUpDown = true;
            }
        }
    }
    else // !pickingLineEnds
    {
        if (down)
        {
            if (pickingUpDown && (x >= targetX || lineBreak))
            {
                // We found the best position candidate: stop here
                point = charId;
                if (!movePointOnly)
                    mark = point;
                direction = None;
                pickingUpDown = false;
                updateSelection();
            }
        }
        else // Up
        {
            if (pickingUpDown && charId < point && (x >= targetX || lineBreak))
            {
                // We found the best position candidate
                previous = charId;
                pickingUpDown = false;
            }
            else if (charId >= point)
            {
                // The last position we had was the right one
                point = previous;
                if (!movePointOnly)
                    mark = point;
                pickingUpDown = false;
                direction = None;
                updateSelection();
            }
        }
    }
}

TAO_END



// ****************************************************************************
//
//    Code generation from text_drawing.tbl
//
// ****************************************************************************

#include "graphics.h"
#include "opcodes.h"
#include "options.h"
#include "widget.h"
#include "types.h"
#include "drawing.h"
#include "layout.h"
#include "module_manager.h"
#include <iostream>


// ============================================================================
//
//    Top-level operation
//
// ============================================================================

#include "widget.h"

using namespace XL;

#include "opcodes_declare.h"
#include "text_drawing.tbl"

namespace Tao
{

#include "text_drawing.tbl"


void EnterTextDrawing()
// ----------------------------------------------------------------------------
//   Enter all the basic operations defined in attributes.tbl
// ----------------------------------------------------------------------------
{
    XL::Context *context = MAIN->context;
#include "opcodes_define.h"
#include "text_drawing.tbl"
}


void DeleteTextDrawing()
// ----------------------------------------------------------------------------
//   Delete all the global operations defined in attributes.tbl
// ----------------------------------------------------------------------------
{
#include "opcodes_delete.h"
#include "text_drawing.tbl"
}

}
