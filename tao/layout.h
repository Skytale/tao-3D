#ifndef LAYOUT_H
#define LAYOUT_H
// ****************************************************************************
//  layout.h                                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//     Drawing object that is used to lay out objects in 2D or 3D space
//     Layouts are used to represent 2D pages, 3D spaces (Z ordering)
//     and text made of a sequence of glyphs
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
//  (C) 2010 Lionel Schaffhauser <lionel@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "drawing.h"
#include "color.h"
#include "justification.h"
#include "tao_gl.h"
#include "application.h"
#include "texture_cache.h"
#include <vector>
#include <set>
#include <QFont>
#include <QEvent>
#include <float.h>
#include "matrix.h"

#define MAX_TEX_UNITS 64

TAO_BEGIN

struct Widget;

struct TextureState
// ----------------------------------------------------------------------------
//   The state of the texture we want to preserve
// ----------------------------------------------------------------------------
{
    TextureState(): wrapS(false), wrapT(false),
                    id(0), unit(0), width(0), height(0),
                    type(GL_TEXTURE_2D), mode(GL_MODULATE),
                    minFilt(TextureCache::instance()->minFilter()),
                    magFilt(TextureCache::instance()->magFilter()) {}

    bool          wrapS, wrapT;
    GLuint        id, unit;
    GLuint        width, height;
    GLenum        type;
    GLenum        mode;
    GLenum        minFilt, magFilt;
};


struct ModelState
// ----------------------------------------------------------------------------
//   The state of the model we want to preserve
// ----------------------------------------------------------------------------
{
    ModelState(): tx(0), ty(0), tz(0),
                  sx(1), sy(1), sz(1),
                  rotation(1, 0, 0, 0) {}

    float tx, ty, tz;     // Translate parameters
    float sx, sy, sz;     // Scaling parameters
    Quaternion rotation;  // Rotation parameters
};


struct LayoutState
// ----------------------------------------------------------------------------
//   The state we want to preserve in a layout
// ----------------------------------------------------------------------------
{
                        LayoutState();
                        LayoutState(const LayoutState &o);

public:
    typedef std::set<QEvent::Type>              qevent_ids;
    typedef std::map<uint,TextureState>         tex_list;


public:
    void                ClearAttributes(bool all = false);
    static text         ToText(qevent_ids & ids);
    static text         ToText(QEvent::Type type);
    void                InheritState(LayoutState *other);
    void                toDebugString(std::ostream &out) const;

public:
    Vector3             offset;
    QFont               font;
    Justification       alongX, alongY, alongZ;
    coord               left, right, top, bottom; // Margins
    scale               visibility;
    scale               lineWidth;
    Color               lineColor;
    Color               fillColor;

    // Textures paramters
    TextureState        currentTexture;
    uint64              textureUnits; //Current used texture units
    tex_list            previousTextures;
    tex_list            fillTextures;

    // Lighting parameters
    uint                lightId;
    uint64              currentLights; //Current used lights
    uint                perPixelLighting;
    uint                programId;

    // Transformations
    double              planarRotation;
    double              planarScale;
    uint                rotationId, translationId, scaleId;
    Matrix4             model;

    // For optimized drawing, we keep track of what changes
    uint64              hasTextureMatrix; // 64 texture units
    bool                printing        : 1;
    bool                hasPixelBlur    : 1; // Pixels not aligning naturally
    bool                hasMatrix       : 1;
    bool                has3D           : 1;
    bool                hasAttributes   : 1;
    bool                hasLighting     : 1;
    bool                hasBlending     : 1;
    bool                hasTransform    : 1;
    bool                hasMaterial     : 1;
    bool                hasDepthAttr    : 1;
    bool                isSelection     : 1;
    bool                groupDrag       : 1;

};


struct Layout : Drawing, LayoutState
// ----------------------------------------------------------------------------
//   A layout is responsible for laying out Drawing objects in 2D or 3D space
// ----------------------------------------------------------------------------
{
    typedef std::list<Drawing *>        Drawings;
    typedef std::vector<Layout *>       Layouts;

public:
                        Layout(Widget *display);
                        Layout(const Layout &o);
                        ~Layout();

    // Drawing interface
    virtual void        Draw(Layout *where);
    virtual void        DrawSelection(Layout *);
    virtual void        Identify(Layout *);
    virtual Box3        Bounds(Layout *);
    virtual Box3        Space(Layout *);

    // Layout interface
    virtual void        Add (Drawing *d);
    virtual Vector3     Offset();
    virtual Layout *    NewChild()       { return new Layout(*this); }
    virtual Layout *    AddChild(uint id = 0,
                                 Tree_p body = 0, Context_p ctx = 0,
                                 Layout *child = NULL);
    virtual void        Clear();
    virtual Widget *    Display()        { return display; }
    virtual void        PolygonOffset();
    virtual void        ClearPolygonOffset();
    virtual uint        Selected();
    virtual uint        ChildrenSelected();

    // Event interface
    virtual bool        Refresh(QEvent *e, double now,
                                Layout *parent=NULL, QString dbg = "");
    virtual void        RefreshLayouts(Layouts &layouts);
    bool                RefreshChildren(QEvent *e, double now, QString debug);
    bool                NeedRefresh(QEvent *e, double when);
    void                RefreshOn(Layout *);
    void                RefreshOn(QEvent::Type type, double when = DBL_MAX);
    void                NoRefreshOn(QEvent::Type type);
    qevent_ids          RefreshEvents();
    double              NextRefresh();

    LayoutState &       operator=(const LayoutState &o);
    virtual void        Inherit(Layout *other);
    void                PushLayout(Layout *where);
    void                PopLayout(Layout *where);
    uint                CharacterId();
    double              PrinterScaling();
    text                PrettyId();

    // Used to optimize away texturing and programs if in Identify
    static bool         InIdentify()    { return inIdentify; }
    
public:
    // OpenGL identification for that shape and for characters within
    uint                id;
    uint                charId;

    GLbitfield glSaveBits()
    {
        GLbitfield bits = 0;
        if (hasAttributes)
            bits |= (GL_LINE_BIT | GL_TEXTURE_BIT);
        if (hasLighting)
            bits |= GL_LIGHTING_BIT;
        if (hasBlending)
            bits |= GL_COLOR_BUFFER_BIT;
        if (hasDepthAttr)
            bits |= GL_DEPTH_BUFFER_BIT;
        return bits;
    }

protected:
    // List of drawing elements
    Drawings            items;
    Widget *            display;
    // Debug: index in parent items (-1 = root layout)
    int                 idx;

public:
    qevent_ids          refreshEvents;
    double              nextRefresh;
    Tree_p              body;
    Context_p           ctx;

public:
    // Static attributes for polygon offset computation
    static int          polygonOffset;
    static scale        factorBase, factorIncrement;
    static scale        unitBase, unitIncrement;
    static bool         inIdentify;
};


struct StereoLayout : Layout
// ----------------------------------------------------------------------------
//   A layout that activates only for some viewpoint
// ----------------------------------------------------------------------------
{
    StereoLayout(const Layout &o, ulong viewpoints = ~0UL)
        : Layout(o), viewpoints(viewpoints) {}

    virtual void        Draw(Layout *where);
    virtual void        DrawSelection(Layout *where);
    bool                Valid(Layout *where);

    ulong       viewpoints;
};


TAO_END

#endif // LAYOUT_H
