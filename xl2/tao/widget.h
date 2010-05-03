#ifndef WIDGET_H
#define WIDGET_H
// ****************************************************************************
//  widget.h                                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//    The primary graphical widget used to display Tao contents
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Lionel Schaffhauser <lionel@taodyne.com>
//  (C) 2010 Catherine Burvelle <cathy@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "main.h"
#include "tao.h"
#include "tao_tree.h"
#include "coords3d.h"
#include "opcodes.h"
#include "drawing.h"
#include "activity.h"
#include "menuinfo.h"
#include "glyph_cache.h"

#include <GL/glew.h>
#include <QtOpenGL>
#include <QImage>
#include <QTimeLine>
#include <QTimer>
#include <QSvgRenderer>
#include <QList>
#include <QColorDialog>
#include <QFontDialog>
#include <iostream>
#include <map>

namespace Tao {

struct Window;
struct FrameInfo;
struct Layout;
struct PageLayout;
struct SpaceLayout;
struct GraphicPath;
struct Repository;
struct Drag;
struct TextSelect;
struct WidgetSurface;

// ----------------------------------------------------------------------------
// Name of fixed menu. Menus then may be retrieved by
//   QMenu * view = window->findChild<QMenu*>(VIEW_MENU_NAME)
// ----------------------------------------------------------------------------
#define FILE_MENU_NAME  "TAO_FILE_MENU"
#define EDIT_MENU_NAME  "TAO_EDIT_MENU"
#define SHARE_MENU_NAME "TAO_SHARE_MENU"
#define VIEW_MENU_NAME  "TAO_VIEW_MENU"
#define HELP_MENU_NAME  "TAO_HELP_MENU"

class Widget : public QGLWidget
// ----------------------------------------------------------------------------
//   This is the widget we use to display XL programs output
// ----------------------------------------------------------------------------
{
    Q_OBJECT
public:
    typedef std::vector<double>   attribute_args;

public:
    Widget(Window *parent, XL::SourceFile *sf = NULL);
    ~Widget();

public slots:
    // Slots
    void        dawdle();
    void        draw();
    void        runProgram();
    void        appFocusChanged(QWidget *prev, QWidget *next);
    void        userMenu(QAction *action);
    bool        refresh(double delay = 0.0);
    void        commitSuccess(QString id, QString msg);
    void        colorChosen(const QColor &);
    void        colorChanged(const QColor &);
    void        updateColorDialog();
    void        fontChosen(const QFont &);
    void        fontChanged(const QFont &);
    void        updateFontDialog();
    void        updateDialogs()                { mustUpdateDialogs = true; }
    void        fileChosen(const QString & filename);
    void        copy();
    void        cut();
    void        paste();
    void        enableAnimations(bool animate);

signals:
    // Signals
    void        copyAvailable(bool yes = true);

public:
    // OpenGL
    void        initializeGL();
    void        resizeGL(int width, int height);
    void        paintGL();
    void        setup(double w, double h, Box *picking = NULL);
    void        setupGL();
    void        identifySelection();
    void        updateSelection();

    // Events
    bool        forwardEvent(QEvent *event);
    bool        forwardEvent(QMouseEvent *event);
    void        keyPressEvent(QKeyEvent *event);
    void        keyReleaseEvent(QKeyEvent *event);
    void        mousePressEvent(QMouseEvent *);
    void        mouseReleaseEvent(QMouseEvent *);
    void        mouseMoveEvent(QMouseEvent *);
    void        mouseDoubleClickEvent(QMouseEvent *);
    void        wheelEvent(QWheelEvent *);
    void        timerEvent(QTimerEvent *);

    // XL program management
    void        updateProgram(XL::SourceFile *sf);
    void        applyAction(XL::Action &action);
    void        reloadProgram(XL::Tree_p newProg = NULL);
    void        renormalizeProgram();
    void        refreshProgram();
    void        markChanged(text reason);
    bool        writeIfChanged(XL::SourceFile &sf);
    bool        doCommit(bool immediate = false);
    Repository *repository();
    Tree_p      get(Tree_p shape, text name, text sh = "shape");
    bool        set(Tree_p shape, text n, Tree_p value, text sh = "shape");
    bool        get(Tree_p shape, text n, XL::TreeList &a, text sh = "shape");
    bool        set(Tree_p shape, text n, XL::TreeList &a, text sh = "shape");
    bool        get(Tree_p shape, text n, attribute_args &a, text sh = "shape");
    bool        set(Tree_p shape, text n, attribute_args &a, text sh = "shape");

    // Timing
    ulonglong   now();
    ulonglong   elapsed(ulonglong since, ulonglong until,
                        bool stats = true, bool show=true);
    bool        timerIsActive()         { return timer.isActive(); }
    bool        hasAnimations(void)     { return animated; }


    // Selection
    enum { CHAR_ID_BIT = 1U<<31, CHAR_ID_MASK = ~CHAR_ID_BIT };
    GLuint      newId()                 { return ++id; }
    GLuint      currentId()             { return id; }
    GLuint      manipulatorId()         { return manipulator; }
    GLuint      selectionCapacity()     { return capacity; }
    GLuint      newCharId(uint ids = 1) { return charId += ids; }
    GLuint      currentCharId()         { return charId; }
    uint        charSelected(uint i)    { return selected(i | CHAR_ID_BIT); }
    uint        charSelected()          { return charSelected(charId); }
    void        selectChar(uint i,uint c){ select(i|CHAR_ID_BIT, c); }
    uint        selected(Tree_p tree)   { return selectionTrees.count(tree); }
    bool        selected()              { return !selectionTrees.empty(); }
    bool        hasSelection()          { return selected(); }
    void        deselect(Tree_p tree)   { selectionTrees.erase(tree); }
    uint        selected(uint i);
    uint        selected(Layout *);
    void        select(uint id, uint count);
    void        deleteFocus(QWidget *widget);
    void        requestFocus(QWidget *widget, coord x, coord y);
    void        recordProjection();
    uint        lastModifiers()         { return keyboardModifiers; }
    Point3      unproject (coord x, coord y, coord z = 0.0);
    Drag *      drag();
    TextSelect *textSelection();
    void        drawSelection(const Box3 &bounds, text name);
    void        drawHandle(const Point3 &point, text name);
    template<class Activity>
    Activity *  active();
    void        checkCopyAvailable();
    bool        canPaste();

    // Text flows and text managemen
    PageLayout*&pageLayoutFlow(text name) { return flows[name]; }
    GlyphCache &glyphs()    { return glyphCache; }

public:
    // XLR entry points
    static Widget *Tao() { return current; }

    // Getting attributes
    Text_p       page(Tree_p self, text name, Tree_p body);
    Text_p       pageLink(Tree_p self, text key, text name);
    Text_p       pageLabel(Tree_p self);
    Integer_p    pageNumber(Tree_p self);
    Integer_p    pageCount(Tree_p self);
    Real_p       pageWidth(Tree_p self);
    Real_p       pageHeight(Tree_p self);
    Real_p       frameWidth(Tree_p self);
    Real_p       frameHeight(Tree_p self);
    Real_p       frameDepth(Tree_p self);
    Real_p       windowWidth(Tree_p self);
    Real_p       windowHeight(Tree_p self);
    Real_p       time(Tree_p self);
    Real_p       pageTime(Tree_p self);

    // Preserving attributes
    Tree_p       locally(Tree_p self, Tree_p t);
    Tree_p       shape(Tree_p self, Tree_p t);

    // Transforms
    Tree_p       rotatex(Tree_p self, real_r rx);
    Tree_p       rotatey(Tree_p self, real_r ry);
    Tree_p       rotatez(Tree_p self, real_r rz);
    Tree_p       rotate(Tree_p self, real_r ra, real_r rx, real_r ry, real_r rz);
    Tree_p       translatex(Tree_p self, real_r x);
    Tree_p       translatey(Tree_p self, real_r y);
    Tree_p       translatez(Tree_p self, real_r z);
    Tree_p       translate(Tree_p self, real_r x, real_r y, real_r z);
    Tree_p       rescalex(Tree_p self, real_r x);
    Tree_p       rescaley(Tree_p self, real_r y);
    Tree_p       rescalez(Tree_p self, real_r z);
    Tree_p       rescale(Tree_p self, real_r x, real_r y, real_r z);

    // Setting attributes
    Name_p       depthTest(Tree_p self, bool enable);
    Tree_p       refresh(Tree_p self, double delay);
    Name_p       fullScreen(Tree_p self, bool fs);
    Name_p       enableAnimations(Tree_p self, bool fs);
    Name_p       toggleFullScreen(Tree_p self);
    Integer_p    polygonOffset(Tree_p self,
                              double f0, double f1, double u0, double u1);

    // Graphic attributes
    Tree_p       lineColor(Tree_p self, double r, double g, double b, double a);
    Tree_p       lineWidth(Tree_p self, double lw);
    Tree_p       lineStipple(Tree_p self, uint16 pattern, uint16 scale);
    Tree_p       fillColor(Tree_p self, double r, double g, double b, double a);
    Tree_p       fillTexture(Tree_p self, text fileName);
    Tree_p       fillTextureFromSVG(Tree_p self, text svg);

    // Generating a path
    Tree_p       newPath(Tree_p self, Tree_p t);
    Tree_p       moveTo(Tree_p self, real_r x, real_r y, real_r z);
    Tree_p       lineTo(Tree_p self, real_r x, real_r y, real_r z);
    Tree_p       curveTo(Tree_p self,
                        real_r cx, real_r cy, real_r cz,
                        real_r x, real_r y, real_r z);
    Tree_p       curveTo(Tree_p self,
                        real_r c1x, real_r c1y, real_r c1z,
                        real_r c2x, real_r c2y, real_r c2z,
                        real_r x, real_r y, real_r z);
    Tree_p       moveToRel(Tree_p self, real_r x, real_r y, real_r z);
    Tree_p       lineToRel(Tree_p self, real_r x, real_r y, real_r z);
    Tree_p       pathTextureCoord(Tree_p self, real_r x, real_r y, real_r r);
    Tree_p       pathColor(Tree_p self, real_r r, real_r g, real_r b, real_r a);
    Tree_p       closePath(Tree_p self);

    // 2D primitive that can be in a path or standalone
    Tree_p       rectangle(Tree_p self, real_r x, real_r y, real_r w, real_r h);
    Tree_p       isoscelesTriangle(Tree_p self,
                                  real_r x, real_r y, real_r w, real_r h);
    Tree_p       rightTriangle(Tree_p self,
                              real_r x, real_r y, real_r w, real_r h);
    Tree_p       ellipse(Tree_p self, real_r x, real_r y, real_r w, real_r h);
    Tree_p       ellipseArc(Tree_p self, real_r x, real_r y, real_r w, real_r h,
                           real_r start, real_r sweep);
    Tree_p       roundedRectangle(Tree_p self,
                                 real_r cx, real_r cy, real_r w, real_r h,
                                 real_r r);
    Tree_p       ellipticalRectangle(Tree_p self,
                                    real_r cx, real_r cy, real_r w, real_r h,
                                    real_r r);
    Tree_p       arrow(Tree_p self, real_r cx, real_r cy, real_r w, real_r h,
                      real_r ax, real_r ary);
    Tree_p       doubleArrow(Tree_p self,
                            real_r cx, real_r cy, real_r w, real_r h,
                            real_r ax, real_r ary);
    Tree_p       starPolygon(Tree_p self,
                            real_r cx, real_r cy, real_r w, real_r h,
                            integer_r p, integer_r q);
    Tree_p       star(Tree_p self, real_r cx, real_r cy, real_r w, real_r h,
                     integer_r p, real_r r);
    Tree_p       speechBalloon(Tree_p self,
                              real_r cx, real_r cy, real_r w, real_r h,
                              real_r r, real_r ax, real_r ay);
    Tree_p       callout(Tree_p self,
                        real_r cx, real_r cy, real_r w, real_r h,
                        real_r r, real_r ax, real_r ay, real_r d);

    Tree_p       debugBinPacker(Tree_p self, uint w, uint h, Tree_p t);
    Tree_p       debugParameters(Tree_p self,
                                double x, double y,
                                double w, double h);

    // 3D primitives
    Tree_p       sphere(Tree_p self,
                       real_r cx, real_r cy, real_r cz,
                       real_r w, real_r, real_r d,
                       integer_r nslices, integer_r nstacks);
    Tree_p       cube(Tree_p self, real_r cx, real_r cy, real_r cz,
                     real_r w, real_r h, real_r d);
    Tree_p       cone(Tree_p self, real_r cx, real_r cy, real_r cz,
                     real_r w, real_r h, real_r d);

    // Text and font
    Tree_p       textBox(Tree_p self,
                        real_r x, real_r y, real_r w, real_r h, Tree_p prog);
    Tree_p       textOverflow(Tree_p self,
                             real_r x, real_r y, real_r w, real_r h);
    Text_p       textFlow(Tree_p self, text name);
    Tree_p       textSpan(Tree_p self, text_r content);
    Tree_p       font(Tree_p self, text family);
    Tree_p       fontSize(Tree_p self, double size);
    Tree_p       fontScaling(Tree_p self, double scaling, double minSize);
    Tree_p       fontPlain(Tree_p self);
    Tree_p       fontItalic(Tree_p self, scale amount = 1);
    Tree_p       fontBold(Tree_p self, scale amount = 1);
    Tree_p       fontUnderline(Tree_p self, scale amount = 1);
    Tree_p       fontOverline(Tree_p self, scale amount = 1);
    Tree_p       fontStrikeout(Tree_p self, scale amount = 1);
    Tree_p       fontStretch(Tree_p self, scale amount = 1);
    Tree_p       justify(Tree_p self, scale amount, uint axis);
    Tree_p       center(Tree_p self, scale amount, uint axis);
    Tree_p       spread(Tree_p self, scale amount, uint axis);
    Tree_p       spacing(Tree_p self, scale amount, uint axis);
    Tree_p       drawingBreak(Tree_p self, Drawing::BreakOrder order);
    Name_p       textEditKey(Tree_p self, text key);

    // Frames and widgets
    Tree_p       status(Tree_p self, text t);
    Tree_p       framePaint(Tree_p self, real_r x, real_r y, real_r w, real_r h,
                           Tree_p prog);
    Tree_p       frameTexture(Tree_p self, double w, double h, Tree_p prog);

    Tree_p       urlPaint(Tree_p self, real_r x, real_r y, real_r w, real_r h,
                         text_p s, integer_p p);
    Tree_p       urlTexture(Tree_p self,
                           double x, double y,
                           Text_p s, Integer_p p);

    Tree_p       lineEdit(Tree_p self, real_r x,real_r y,
                         real_r w,real_r h, text_p s);
    Tree_p       lineEditTexture(Tree_p self, double x, double y, Text_p s);

    Tree_p       abstractButton(Tree_p self, Text_p name,
                               real_r x, real_r y, real_r w, real_r h);
    Tree_p       pushButton(Tree_p self, real_r x, real_r y, real_r w, real_r h,
                           text_p name, text_p lbl, Tree_p act);
    Tree_p       pushButtonTexture(Tree_p self, double w, double h,
                                  text_p name, Text_p lbl, Tree_p act);
    Tree_p       radioButton(Tree_p self, real_r x,real_r y, real_r w,real_r h,
                            text_p name, text_p lbl,
                            Text_p selected, Tree_p act);
    Tree_p       radioButtonTexture(Tree_p self, double w, double h,
                                   text_p name, Text_p lbl,
                                   Text_p selected, Tree_p act);
    Tree_p       checkBoxButton(Tree_p self,
                               real_r x,real_r y, real_r w, real_r h,
                               text_p name, text_p lbl, Text_p  marked,
                               Tree_p act);
    Tree_p       checkBoxButtonTexture(Tree_p self,
                                      double w, double h,
                                      text_p name, Text_p lbl,
                                      Text_p  marked, Tree_p act);
    Tree_p       buttonGroup(Tree_p self, bool exclusive, Tree_p buttons);
    Tree_p       setButtonGroupAction(Tree_p self, Tree_p action);

    Tree_p       colorChooser(Tree_p self, text name, Tree_p action);
    Tree_p       colorChooser(Tree_p self,
                             real_r x, real_r y, real_r w, real_r h,
                             Tree_p action);
    Tree_p       colorChooserTexture(Tree_p self,
                                    double w, double h,
                                    Tree_p action);

    Tree_p       fontChooser(Tree_p self, Tree_p action);
    Tree_p       fontChooser(Tree_p self,
                            real_r x, real_r y, real_r w, real_r h,
                            Tree_p action);
    Tree_p       fontChooserTexture(Tree_p self,
                                   double w, double h,
                                   Tree_p action);

    Tree_p       fileChooser(Tree_p self, Tree_p action);
    Tree_p       fileChooser(Tree_p self,
                            real_r x, real_r y, real_r w, real_r h,
                            Tree_p action);
    Tree_p       fileChooserTexture(Tree_p self,
                                    double w, double h,
                                    Tree_p action);
    Tree_p       setFileDialogAction(Tree_p self, Tree_p action);
    Tree_p       setFileDialogDirectory(Tree_p self, text dirname);
    Tree_p       setFileDialogFilter(Tree_p self, text filters);
    Tree_p       setFileDialogLabel(Tree_p self, text label, text value);

    Tree_p       groupBox(Tree_p self,
                         real_r x,real_r y, real_r w,real_r h,
                         text_p lbl, Tree_p buttons);
    Tree_p       groupBoxTexture(Tree_p self,
                                double w, double h,
                                Text_p lbl);

    Tree_p       videoPlayer(Tree_p self,
                            real_r x, real_r y, real_r w, real_r h, Text_p url);

    Tree_p       videoPlayerTexture(Tree_p self, real_r w, real_r h, Text_p url);

    Tree_p       image(Tree_p self, real_r x, real_r y, real_r w, real_r h,
                      text filename);

    // Menus and widgets
    Tree_p       runtimeError(Tree_p self, text msg, Tree_p src);
    Tree_p       menuItem(Tree_p self, text name, text lbl, text iconFileName,
                         bool isCheckable, Text_p isChecked, Tree_p t);
    Tree_p       menu(Tree_p self, text name, text lbl, text iconFileName,
                     bool isSubmenu=false);

    // The location is the prefered location for the toolbar.
    // The supported values are North, East, South, West or N, E, S, W
    Tree_p       toolBar(Tree_p self, text name, text title, bool isFloatable,
                        text location);

    Tree_p       menuBar(Tree_p self);
    Tree_p       separator(Tree_p self);

    // Tree management
    Name_p       insert(Tree_p self, Tree_p toInsert);
    void        deleteSelection();
    Name_p       deleteSelection(Tree_p self, text key);
    Name_p       setAttribute(Tree_p self, text name, Tree_p attribute, text sh);

    // Unit conversionsxo
    Real_p       fromCm(Tree_p self, double cm);
    Real_p       fromMm(Tree_p self, double mm);
    Real_p       fromIn(Tree_p self, double in);
    Real_p       fromPt(Tree_p self, double pt);
    Real_p       fromPx(Tree_p self, double px);

private:
    friend class Window;
    friend class Activity;
    friend class Selection;
    friend class Drag;
    friend class TextSelect;
    friend class Manipulator;
    friend class ControlPoint;

    typedef XL::LocalSave<QEvent *>             EventSave;
    typedef std::map<GLuint, uint>              selection_map;
    typedef std::map<text, PageLayout*>         flow_map;
    typedef std::map<text, text>                page_map;

    // XL Runtime
    XL::SourceFile       *xlProgram;
    bool                  inError;
    bool                  mustUpdateDialogs;

    // Rendering
    SpaceLayout *         space;
    Layout *              layout;
    GraphicPath *         path;
    scale                 pageW, pageH;
    text                  flowName;
    flow_map              flows;
    text                  pageName, lastPageName;
    page_map              pageLinks;
    uint                  pageId, pageShown, pageTotal;
    Tree_p                 pageTree;
    Tree_p                 currentShape;
    QGridLayout *         currentGridLayout;
    GroupInfo   *         currentGroup;
    GlyphCache            glyphCache;

    // Selection
    Activity *            activities;
    GLuint                id, charId, capacity, manipulator;
    selection_map         selection, savedSelection;
    std::set<Tree_p >      selectionTrees, selectNextTime;
    bool                  wasSelected;
    QEvent *              event;
    QWidget *             focusWidget;
    GLdouble              focusProjection[16], focusModel[16];
    GLint                 focusViewport[4];
    uint                  keyboardModifiers;

    // Menus and widgets
    QMenu                *currentMenu;
    QMenuBar             *currentMenuBar;
    QToolBar             *currentToolBar;
    QVector<MenuInfo*>    orderedMenuElements;
    int                   order;
    XL::Tree_p            colorAction, fontAction;
    text                  colorName;

    // Timing
    QTimer                timer, idleTimer;
    double                pageStartTime, pageRefresh, frozenTime;
    ulonglong             tmin, tmax, tsum, tcount;
    ulonglong             nextSave, nextCommit, nextSync, nextPull;
    bool                  animated;

    static Widget *       current;
    static QColorDialog * colorDialog;
    static QFontDialog *  fontDialog;
    static QFileDialog *  fileDialog;
           QFileDialog *  currentFileDialog;
    static double         zNear, zFar;

    std::map<text, QFileDialog::DialogLabel> toDialogLabel;
private:
    void        updateFileDialog(Tree_p properties);

};


template<class ActivityClass>
inline ActivityClass *Widget::active()
// ----------------------------------------------------------------------------
//   Return an activity of the given type
// ----------------------------------------------------------------------------
{
    for (Activity *a = activities; a; a = a->next)
        if (ActivityClass *result = dynamic_cast<ActivityClass *> (a))
            return result;
    return NULL;
}



// ============================================================================
//
//    Simple utility functions
//
// ============================================================================

inline void glShowErrors()
// ----------------------------------------------------------------------------
//   Display pending GL errors
// ----------------------------------------------------------------------------
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        std::cerr << "GL Error: " << (char *) gluErrorString(err) << "\n";
        err = glGetError();
    }
}


inline QString Utf8(text utf8, uint index = 0)
// ----------------------------------------------------------------------------
//    Convert our internal UTF-8 encoded strings to QString format
// ----------------------------------------------------------------------------
{
    kstring data = utf8.data();
    uint len = utf8.length();
    len = len > index ? len - index : 0;
    index = index < len ? index : 0;
    return QString::fromUtf8(data + index, len);
}


inline double CurrentTime()
// ----------------------------------------------------------------------------
//    Return the current time
// ----------------------------------------------------------------------------
{
    QTime t = QTime::currentTime();
    double d = (3600.0	 * t.hour()
                + 60.0	 * t.minute()
                +	   t.second()
                +  0.001 * t.msec());
    return d;
}

#undef TAO // From the command line
#define TAO(x)  (Tao::Widget::Tao() ? Tao::Widget::Tao()->x : 0)
#define RTAO(x) return TAO(x)



// ============================================================================
//
//   Action that returns a tree where all the selected trees are removed
//
// ============================================================================

struct DeleteSelectionAction : XL::TreeClone
// ----------------------------------------------------------------------------
//    A specialized clone action that doesn't copy selected trees
// ----------------------------------------------------------------------------
{
    DeleteSelectionAction(Widget *widget): widget(widget) {}
    XL::Tree_p DoInfix(XL::Infix_p what)
    {
        if (what->name == "\n" || what->name == ";")
        {
            if (widget->selected(what->left))
            {
                if (widget->selected(what->right))
                    return NULL;
                return what->right->Do(this);
            }
            if (widget->selected(what->right))
                return what->left->Do(this);
        }
        XL::Tree_p left = what->left->Do(this);
        XL::Tree_p right = what->right->Do(this);
        if (left && right)
            return new XL::Infix(what->name, left, right, what->Position());
        else if (left)
            return left;
        return right;
    }
    Widget *widget;
};


struct InsertAtSelectionAction : XL::TreeClone
// ----------------------------------------------------------------------------
//    A specialized clone action that inserts an input
// ----------------------------------------------------------------------------
{
    InsertAtSelectionAction(Widget *widget,
                            XL::Tree_p toInsert, XL::Tree_p parent)
        : widget(widget), toInsert(toInsert), parent(parent) {}


    XL::Tree_p DoName(XL::Name_p what)
    {
        if (what == parent)
            parent = NULL;
        return XL::TreeClone::DoName(what);
    }

    XL::Tree_p DoPrefix(XL::Prefix_p what)
    {
        if (what == parent)
            parent = NULL;
        return XL::TreeClone::DoPrefix(what);
    }

    XL::Tree_p DoPostfix(XL::Postfix_p what)
    {
        if (what == parent)
            parent = NULL;
        return XL::TreeClone::DoPostfix(what);
    }

    XL::Tree_p DoBlock(XL::Block_p what)
    {
        if (what == parent)
            parent = NULL;
        return XL::TreeClone::DoBlock(what);
    }

    XL::Tree_p DoInfix(XL::Infix_p what)
    {
        if (what == parent)
            parent = NULL;

        if (!parent)
        {
            if (what->name == "\n" || what->name == ";")
            {
                // Check if we hit the selection. If so, insert
                if (toInsert && widget->selected(what->left))
                {
                    XL::Tree_p ins = toInsert;
                    toInsert = NULL;
                    return new XL::Infix("\n", ins, what->Do(this));
                }
            }
        }
        return XL::TreeClone::DoInfix(what);
    }
    Widget   *widget;
    XL::Tree_p toInsert;
    XL::Tree_p parent;
};


struct SetAttributeAction : XL::Action
// ----------------------------------------------------------------------------
//    Copy the inserted item as attribute in all selected items
// ----------------------------------------------------------------------------
{
    SetAttributeAction(text name, XL::Tree_p attribute,
                       Widget *widget, text shape = "shape")
        : name(name), attribute(attribute), widget(widget), shape(shape) {}

    XL::Tree_p Do(XL::Tree_p what)
    {
        if (widget->selected(what))
            widget->set(what, name, attribute, shape);
        return what;
    }

    text      name;
    XL::Tree_p attribute;
    Widget   *widget;
    text      shape;
};


struct NameToNameReplacement : XL::TreeClone
// ----------------------------------------------------------------------------
//    Replace specific names with names (e.g. alternate spellings)
// ----------------------------------------------------------------------------
{
    NameToNameReplacement(){}

    XL::Tree_p    DoName(XL::Name_p what);
    XL::Tree_p    Replace(XL::Tree_p original);
    text &      operator[] (text index)         { return map[index]; }

    std::map<text, text> map;
};


struct NameToTextReplacement : NameToNameReplacement
// ----------------------------------------------------------------------------
//    Replace specific names with a text
// ----------------------------------------------------------------------------
{
    NameToTextReplacement(): NameToNameReplacement() {}
    XL::Tree_p  DoName(XL::Name_p what);
};

} // namespace Tao



// ============================================================================
//
//   Qt interface for XL types
//
// ============================================================================

#define TREEPOINTER_TYPE 383 // (QVariant::UserType | 0x100)
Q_DECLARE_METATYPE(XL::Tree_p)

#endif // TAO_WIDGET_H
