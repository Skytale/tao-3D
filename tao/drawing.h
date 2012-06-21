#ifndef DRAWING_H
#define DRAWING_H
// ****************************************************************************
//  drawing.h                                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//      Elements that can be drawn on a 2D layout
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

#include "coords3d.h"
#include "tao.h"
#include "tao_tree.h"
#include "graphic_state.h"


TAO_BEGIN
struct Layout;

struct Drawing
// ----------------------------------------------------------------------------
//   Something that can be drawn in a 2D or 3D layout
// ----------------------------------------------------------------------------
//   Draw() draws the shape in the given layout
//   Bounds() returns the untransformed bounding box for the shape
//   Space() returns the untransformed space desired around object
//   For instance, for text, Space() considers font line height, not Bounds()
{
                        Drawing();
                        Drawing(const Drawing &);
    virtual             ~Drawing();

    virtual void        Draw(Layout *);
    virtual void        DrawSelection(Layout *);
    virtual void        Evaluate(Layout *);
    virtual void        Identify(Layout *);
    virtual Box3        Bounds(Layout *);
    virtual Box3        Space(Layout *);
    virtual Tree *      Source();

    enum BreakOrder
    {
        NoBreak,
        CharBreak, WordBreak, SentenceBreak, LineBreak, ParaBreak,
        ColumnBreak, PageBreak,
        AnyBreak
    };
    virtual Drawing *   Break(BreakOrder &order, uint &size);
    virtual scale       TrailingSpaceSize(Layout *);
    virtual bool        IsAttribute();

    static uint count;
};

TAO_END

#endif // DRAWING_H
