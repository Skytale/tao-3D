#ifndef LIGHTING_H
#define LIGHTING_H
// ****************************************************************************
//  lighting.h                                                      Tao project
// ****************************************************************************
//
//   File Description:
//
//     Lighting attributes
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
//  (C) 2010 Lionel Schaffhauser <lionel@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "drawing.h"
#include "color.h"
#include "attributes.h"
#include "tao_gl.h"
#include "info_trash_can.h"
#include <vector>
#include <QGLShaderProgram>

TAO_BEGIN

struct Lighting : Attribute
// ----------------------------------------------------------------------------
//   Base class for all lighting attributes
// ----------------------------------------------------------------------------
{
    Lighting(): Attribute() {}
};

struct ShaderProgramInfo : XL::Info, InfoTrashCan
// ----------------------------------------------------------------------------
//   Hold info associated to a tree
// ----------------------------------------------------------------------------
{
    ShaderProgramInfo(QGLShaderProgram *program): program(program) {}
    ~ShaderProgramInfo() { delete program; }
    virtual void Delete() { trash.push_back(this); }
    typedef QGLShaderProgram *data_t;
    operator data_t() { return program; }
    QGLShaderProgram *program;
};



struct ShaderProgram : Lighting
// ----------------------------------------------------------------------------
//   Activate a shader program
// ----------------------------------------------------------------------------
{
    ShaderProgram(QGLShaderProgram *program): program(program) {}
    virtual void Draw(Layout *where);
    QGLShaderProgram *program;
};


struct ShaderUniformInfo : XL::Info
// ----------------------------------------------------------------------------
//   Record information about a uniform variable
// ----------------------------------------------------------------------------
{
    ShaderUniformInfo(GLuint id): id(id) {}
    GLuint id;
};


struct ShaderAttributeInfo : XL::Info
// ----------------------------------------------------------------------------
//   Record information about an attribute variable
// ----------------------------------------------------------------------------
{
    ShaderAttributeInfo(GLuint id): id(id) {}
    GLuint id;
};


struct ShaderValue : Lighting
// ----------------------------------------------------------------------------
//   Set a shader uniform or attribute value
// ----------------------------------------------------------------------------
{
    typedef std::vector<float> Values;
    ShaderValue(Name_p name, Values values): name(name), values(values) {}
    virtual void Draw(Layout *where);
    Name_p name;
    Values values;
};

struct PerPixelLighting : Lighting
// ----------------------------------------------------------------------------
//   Enable or disable per pixel lighting
// ----------------------------------------------------------------------------
{
    PerPixelLighting(bool enable = true);
    ~PerPixelLighting();
    virtual void Draw(Layout *where);
    bool enable;
    ShaderProgram* shader;

    static QGLShaderProgram* pgm;
    static bool failed;
};

struct LightId : Lighting
// ----------------------------------------------------------------------------
//   Select which light we are using
// ----------------------------------------------------------------------------
{
    LightId(uint id, bool enable);
    ~LightId();
    virtual void Draw(Layout *where);
    uint id;
    bool enable;
    PerPixelLighting* perPixelLighting;
};


struct Light : Lighting
// ----------------------------------------------------------------------------
//   Light attribute (glLight)
// ----------------------------------------------------------------------------
{
    typedef std::vector<GLfloat> Arguments;
    Light(GLenum f, Arguments &a): Lighting(), function(f), args(a) {}
    Light(GLenum f, GLfloat a): Lighting(), function(f), args()
    {
        args.push_back(a);
    }
    Light(GLenum f, GLfloat x, GLfloat y, GLfloat z):
        Lighting(), function(f), args()
    {
        args.push_back(x);
        args.push_back(y);
        args.push_back(z);
    }
    Light(GLenum f, GLfloat r, GLfloat g, GLfloat b, GLfloat a):
        Lighting(), function(f), args()
    {
        args.push_back(r);
        args.push_back(g);
        args.push_back(b);
        args.push_back(a);
    }
    virtual void Draw(Layout *where);
    GLenum      function;
    Arguments   args;
};


struct Material : Light
// ----------------------------------------------------------------------------
//   Material attribute (glMaterial)
// ----------------------------------------------------------------------------
{
    Material(GLenum fc, GLenum f, GLfloat a): Light(f, a), face(fc) {}
    Material(GLenum fc, GLenum f, GLfloat a, GLfloat b, GLfloat c, GLfloat d)
        : Light(f, a, b, c, d), face(fc) {}
    virtual void Draw(Layout *where);
    GLenum      face;
};



// ============================================================================
// 
//   Entering lighting primitives in the symbols table
// 
// ============================================================================

extern void EnterLighting();
extern void DeleteLighting();

TAO_END

#endif // LIGHTING_H
