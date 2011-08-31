#ifndef TAO_MODULE_API_H
#define TAO_MODULE_API_H
// ****************************************************************************
//  module_api.h                                                   Tao project
// ****************************************************************************
//
//   File Description:
//
//    Interface between the Tao runtime and native modules
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
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "tao/module_info.h"
#include "coords3d.h"

// ========================================================================
//
//   Interface versioning
//
// ========================================================================

// The Tao module loader uses a versioning system similar to libtool's:
// http://www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html
// The loader will refuse to load a module library that does not pass the
// version compatibility check.
//
// Here are the rules when changing these numbers:
//
// - Update the version information only immediately before a public release
// - [ANY CHANGE] If any interfaces have been added, removed, or changed
//   since the last update, increment current.
// - [COMPATIBLE CHANGE] If any interfaces have been added since the last
//   public release, then increment age.
// - [INCOMPATIBLE CHANGE] If any interfaces have been removed or changed
//   since the last public release, then set age to 0.

#define TAO_MODULE_API_CURRENT   9
#define TAO_MODULE_API_AGE       0

// ========================================================================
//
//   Tao runtime interface
//
// ========================================================================

namespace XL
{
    struct Real;
}

namespace Tao {

struct ModuleApi
// ------------------------------------------------------------------------
//   API exported by the Tao runtime for use by modules
// ------------------------------------------------------------------------
{
    typedef void (*render_fn)(void *arg);
    typedef void (*delete_fn)(void *arg);

    // Add a rendering callback to the current layout. Callback will be
    // invoked when it's time to draw the layout.
    // This function is typically used by XL primitives that need to draw
    // something using OpenGL calls.
    bool (*scheduleRender)(render_fn callback, void *arg);

    // Request that current layout be refreshed on specified event
    // If event_type is QEvent::Timer, next_refresh is used to specify when
    // the next refresh should occur:
    //   - if next_refresh is -1.0, refresh will occur after the
    // "default refresh" period (cf. builtin default_refresh). Unless the
    // default_refresh value is changed, this means: as soon as possible.
    //   - if next_refresh is not -1.0, it is the absolute date of the
    // next refresh. Use currentTime() if needed, for instance to request
    // a refresh in 5 seconds, set next_refresh = currentTime() + 5.0.
    // When event_type is not QEvent::Timer, next_refresh is ignored.
    bool (*refreshOn)(int event_type, double next_refresh);

    // Return the current time, in seconds. The returned value normally
    // has millisecond precision.
    double (*currentTime)();

    // Like scheduleRender, but current layout takes ownership of arg:
    // when layout is destroyed, delete_fn is called with arg.
    bool (*addToLayout)(render_fn callback, void *arg, delete_fn del);

    // Show a control box to manipulate the object
    bool (*addControlBox)(XL::Real *x, XL::Real *y, XL::Real *z,
                          XL::Real *w, XL::Real *h, XL::Real *d);

    // Allow to enable specified texture coordinates before a drawing
    // according to current application parameters.
    // After drawing, texture coordinates have to be unbind.
    bool (*EnableTexCoords)(double* texCoord);

    // Allow to disable texture coordinates after a drawing.
    bool (*DisableTexCoords)();

    // Allow to bind a new texture in Tao thanks to its id and its type.
    bool (*BindTexture)(unsigned int id, unsigned int type);

    // Allow to apply current textures during a drawing.
    bool (*SetTextures)();

    // Allow to set fill color during a drawing according
    // to the current layout attributes.
    bool (*SetFillColor)();

    // Allow to set line color during a drawing according
    // to the current layout attributes.
    bool (*SetLineColor)();

    // Allow to enable or deactivate pixel blur
    // on textures of the current layout.
    // It corresponds to GL_LINEAR/GL_NEAREST parameters.
    bool (*HasPixelBlur)(bool enable);

    // ------------------------------------------------------------------------
    //   API for display modules
    // ------------------------------------------------------------------------

    // In the following protoypes, obj is the value returned by the
    // display_use_fn function.
    typedef void   (*display_fn)(void *obj);
    typedef void * (*display_use_fn)();
    typedef void   (*display_unuse_fn)(void *obj);
    typedef bool   (*display_setopt_fn)(void *obj, std::string name,
                                        std::string val);
    typedef std::string
                   (*display_getopt_fn)(void *obj, std::string name);

    // Register a display function (if module is a display module).
    // Display can later be activated by primitive: display_function <name>
    // Returns true on success, false on error (e.g., name already used)
    //   - fn is called for each frame to be displayed
    //   - use is called once when fn is about to be used.
    //     Return (void *)(~0L) to indicated an error.
    //   - unuse is called when Tao stops using fn
    //   - setopt is called when Tao needs to set a display option
    //   - getopt is called when Tao needs to get a display option
    bool (*registerDisplayFunction)(std::string name, display_fn fn,
                                    display_use_fn use,
                                    display_unuse_fn unuse,
                                    display_setopt_fn setopt,
                                    display_getopt_fn getopt);

    // Create another name for an existing display function
    bool (*registerDisplayFunctionAlias)(std::string name,
                                         std::string existing);

    // Call glClearColor() with the color currently specified by the program.
    void (*setGlClearColor)();

    // Setup default OpenGL parameters before drawing.
    void (*setupGl)();

    // Display all pending OpenGL errors (if any), in the error window.
    void (*showGlErrors)();

    // Switch GL_STEREO on or off for the current OpenGL display context.
    // The current context becomes invalid and a new one is created with
    // or without stereo buffers.
    // Returns true on success.
    bool (*setStereo)(bool on);

    // setProjectionMatrix, setModelViewMatrix
    //
    // Helper functions to define the geometry for a given camera.
    // Cameras lie on a straight line (not on an arc) and are spaced evenly by
    // the eyeSeparation() distance. The center of the camera segment is the
    // 'pos' point returned by getCamera(pos, target, up). Cameras are globally
    // aimed at the 'target' point, but they are parallel -- there is no inward
    // rotation or vergence point.
    // This effectively implements the "asymmetric frustum parallel axis"
    // projection method, which is recommended for stereoscopic and
    // autostereoscopic setups.
    //
    // w and h are the width and height in pixels of the frustum
    // plane at the target point. numCameras is the total number of cameras,
    // i is the camera number for which the view is to be rendered (i must be
    // between 1 and numCameras).
    void (*setProjectionMatrix)(int w, int h, int i, int numCameras);
    void (*setModelViewMatrix)(int i, int numCameras);

    // Draw the current page.
    void (*drawScene)();

    // Draw object selection.
    void (*drawSelection)();

    // Draw activities (command menu, selection rectangle, display
    // statistics)
    void (*drawActivities)();

    // Get camera characteristics. pos, target and/or up may be NULL.
    void (*getCamera)(Point3 *pos, Point3 *target, Vector3 *up);

    // The height, in pixels, of the image to be rendered.
    int  (*renderHeight)();

    // The width, in pixels, of the image to be rendered.
    int  (*renderWidth)();

    // The z coordinate of the near clipping plane.
    double (*zNear)();

    // The z coordinate of the far clipping plane.
    double (*zFar)();

    // The current zoom factor.
    double (*zoom)();

    // For stereoscopic or multi-view settings: the distance between
    // the left and the right eye (or camera).
    double (*eyeSeparation)();

    // Tell Tao if the current point of view is to be used for selection/
    // mouse activities. When rendering multiple views per frame, you
    // normally set this for only one view.
    // When this setting is true, the drawScene() call will unproject the
    // mouse coordinates into the 3D space.
    void   (*doMouseTracking)(bool on);

    // Override viewport setting during mouse tracking.
    // This viewport is used in drawScene() when doMouseTracking(true), and
    // only for the current frame.
    // The current GL viewport is used by default.
    void   (*setMouseTrackingViewport)(int x, int y, int w, int h);

    // ------------------------------------------------------------------------
    //   Rendering to framebuffer/texture
    // ------------------------------------------------------------------------

    typedef struct fbo_s fbo;

    // Create a new framebuffer object.
    ModuleApi::fbo *   (*newFrameBufferObject)(uint w, uint h);

    // Delete a framebuffer object.
    void               (*deleteFrameBufferObject)(ModuleApi::fbo * obj);

    // Resize a framebuffer object.
    void               (*resizeFrameBufferObject)(ModuleApi::fbo * obj,
                                                      uint w, uint h);

    // Make a framebuffer object the current rendering target.
    // After this call, OpenGL will draw into the framebuffer.
    void               (*bindFrameBufferObject)(ModuleApi::fbo * obj);

    // Switch OpenGL rendering back to the default rendering target.
    void               (*releaseFrameBufferObject)(ModuleApi::fbo * obj);

    // Make a framebuffer object available as a texture.
    // After this call, the texture is bound to target GL_TEXTURE_2D
    // and target is enabled. That is:
    //   glBindTexture(GL_TEXTURE_2D, id);
    //   glEnable(GL_TEXTURE_2D);
    unsigned int       (*frameBufferObjectToTexture)(ModuleApi::fbo * obj);
};

}

// ========================================================================
//
//   Module interface
//
// ========================================================================

namespace XL
{
    struct Context;
}

namespace Tao
{
    typedef int (*module_init_fn)   (const Tao::ModuleApi *,
                                     const Tao::ModuleInfo *);
    typedef int (*enter_symbols_fn) (XL::Context *);
    typedef int (*delete_symbols_fn)(XL::Context *);
    typedef int (*module_exit_fn)   ();
}

extern "C"
// ------------------------------------------------------------------------
//   API exported by modules for use by the Tao runtime
// ------------------------------------------------------------------------
{
    // Called once immediately after the module library is loaded.
    // Return 0 on success.
    // [Optional]
    // [ModuleInfo is only valable during this call]
    int module_init(const Tao::ModuleApi *a, const Tao::ModuleInfo *m);

    // Called when module is imported to let the module extend the XL symbol
    // table (for instance, add new XL commands) in a given context.
    // Return 0 on success.
    // [Optional]
    // [May be automatically generated by the tbl_gen script]
    int enter_symbols(XL::Context *c);

    // Called once before module is unloaded to let module remove its
    // primitives from the XL symbol table.
    // Return 0 on success.
    // [Optional]
    // [May be automatically generated by the tbl_gen script]
    int delete_symbols(XL::Context *c);

    // Called when the module library is about to be unloaded.
    // Return 0 on success.
    // [Optional]
    int module_exit();
}

#endif // TAO_MODULE_API_H
