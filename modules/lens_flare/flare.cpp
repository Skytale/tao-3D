// ****************************************************************************
//  flare.cpp                                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//    Lens flare implementation.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include "flare.h"

const ModuleApi *LensFlare::tao = NULL;

// ============================================================================
//
//    Lens Flare
//
// ============================================================================

LensFlare::LensFlare()
// ----------------------------------------------------------------------------
//   Construction
// ----------------------------------------------------------------------------
    : query(0), depth_test(true), test_size(5), target(0, 0, 0), source(0, 0, 0)
{
}


LensFlare::~LensFlare()
// ----------------------------------------------------------------------------
//   Destruction
// ----------------------------------------------------------------------------
{
    if (query)
        glDeleteQueries(1, &query);
}


void LensFlare::render_callback(void *arg)
// ----------------------------------------------------------------------------
//   Rendering callback: call the render function for the object
// ----------------------------------------------------------------------------
{
    ((LensFlare *)arg)->Draw();
}


void LensFlare::delete_callback(void *arg)
// ----------------------------------------------------------------------------
//   Delete callback: destroy object
// ----------------------------------------------------------------------------
{
    delete (LensFlare *)arg;
}


void LensFlare::setTarget(Vector3 position)
// ----------------------------------------------------------------------------
//   Define position of the lens flare target
// ----------------------------------------------------------------------------
{
    target = position;
}


void LensFlare::setSource(Vector3 position)
// ----------------------------------------------------------------------------
//   Define position of the lens flare source light
// ----------------------------------------------------------------------------
{
    source = position;
}


void LensFlare::enableDephTest(bool enable)
// ----------------------------------------------------------------------------
//   Enable or disable manual depth test for the lens flare
// ----------------------------------------------------------------------------
{
    if(!query)
        glGenQueries(1, &query);
    depth_test = enable;
}

void LensFlare::addFlare(GLuint id, float location, float scale,
                         GLfloat r, GLfloat g, GLfloat b, GLfloat a)
// ----------------------------------------------------------------------------
//   Create a new flare and add it to the others.
// ----------------------------------------------------------------------------
{
    Flare flare;

    // Define flare paramaters
    flare.id = id;
    flare.loc = location;
    flare.scale = scale;
    flare.color[0] = r;
    flare.color[1] = g;
    flare.color[2] = b;
    flare.color[3] = a;

    // Add to the flares list for drawing
    lens_flare.push_back(flare);
}

void LensFlare::Draw()
// ----------------------------------------------------------------------------
//   Draw the lens flare
// ----------------------------------------------------------------------------
//   We draw a lens flare centered at the source position and heading
//   toward the defined target.
{
    // Manually determine if the source is occluded by a previous object.
    // If it is, we draw no one of the flares.
    bool occluded = isOccluded(source);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Compute lens direction
    Vector3 lens_dir = target  - source;

    glDepthMask(GL_FALSE);
    for(uint i = 0; i < lens_flare.size(); i++)
    {
        // Draw flares at the source position without depth test
        if(lens_flare[i].loc == 0)
        {
            // Interpolate position of the current flare
            Vector3 pos = source + lens_flare[i].loc * lens_dir;
            DrawFlare(lens_flare[i], pos);
        }
        else if(! occluded)
        {
            // Disable current depth_test to avoid
            // display problem with flares
            glDisable(GL_DEPTH_TEST);

            // Interpolate position of the current flare
            Vector3 pos = source + lens_flare[i].loc * lens_dir;
            DrawFlare(lens_flare[i], pos);

            // Restore OpenGL depth test
            glEnable(GL_DEPTH_TEST);
        }
    }
    glDepthMask(GL_TRUE);

    // Restore previous bend settings.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}


void LensFlare::DrawFlare(Flare flare, Vector3 pos)
// ----------------------------------------------------------------------------
//   Draw a flare at the specified position.
// ----------------------------------------------------------------------------
{
    float scale = flare.scale;
    // General scaling rate of flares textures of size 256 * 256
    scale *= 128;

    tao->textureCacheBind(flare.id);
    tao->textureCacheSetMinMagFilters(flare.id);
    tao->SetShader(0);
    glColor4f(flare.color[0],
              flare.color[1],
              flare.color[2],
              flare.color[3]);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(pos.x - scale, pos.y - scale, pos.z);

    glTexCoord2f(1.0, 0.0);
    glVertex3f(pos.x + scale, pos.y - scale, pos.z);

    glTexCoord2f(1.0, 1.0);
    glVertex3f(pos.x + scale, pos.y  + scale, pos.z);

    glTexCoord2f(0.0, 1.0);
    glVertex3f(pos.x - scale, pos.y + scale, pos.z);

    glEnd();
}

bool LensFlare::isOccluded(Vector3 p)
// ----------------------------------------------------------------------------
//   Define a manual depth test in order to determine if
//   a lens flare source is occluded by an other object in the scene
// ----------------------------------------------------------------------------
{
    if(depth_test)
    {
        GLuint result = 0;

        glBeginQuery(GL_SAMPLES_PASSED, query);
        glBegin(GL_QUADS);

        // Draw sun flare
        glVertex3f(p.x - test_size, p.y - test_size, p.z);
        glVertex3f(p.x + test_size, p.y - test_size, p.z);
        glVertex3f(p.x + test_size, p.y + test_size, p.z);
        glVertex3f(p.x - test_size, p.y + test_size, p.z);

        glEnd();
        glEndQuery(GL_SAMPLES_PASSED);
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);

        if(result <= 0)
            return true;
    }

    return false;
}
