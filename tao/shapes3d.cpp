// ****************************************************************************
//  shapes3d.cpp                                                    Tao project
// ****************************************************************************
// 
//   File Description:
// 
//     Drawing of elementary 3D geometry shapes
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

#include "shapes3d.h"
#include "layout.h"
#include "widget.h"
#include "tao_gl.h"
#include "application.h"

TAO_BEGIN

void Shape3::DrawSelection(Layout *layout)
// ----------------------------------------------------------------------------
//   Draw a selection for 3-dimensional objects
// ----------------------------------------------------------------------------
{
    Widget *widget = layout->Display();
    uint sel = widget->selected(layout);
    if (sel)
    {
        Box3 bounds = Bounds(layout);
        XL::Save<Point3> zeroOffset(layout->offset, Point3(0,0,0));
        widget->drawSelection(layout, bounds, "3D_selection", 0);
    }
}

    
bool Shape3::setFillColor(Layout *where)
// ----------------------------------------------------------------------------
//    Set the fill color and texture according to the layout attributes
// ----------------------------------------------------------------------------
//    This is like in Shape, except that we don't increment polygon offset
{
    // Check if we have a non-transparent fill color
    if (where)
    {
        Color &color = where->fillColor;
        scale v = where->visibility * color.alpha;
        if (v > 0.0)
        {
            if (!where->hasMaterial)
                glColor4f(color.red, color.green, color.blue, v);
            return true;
        }
    }
    return false;
}


bool Shape3::setLineColor(Layout *where)
// ----------------------------------------------------------------------------
//    Set the outline color according to the layout attributes
// ----------------------------------------------------------------------------
//    This is like in Shape, except that we don't increment polygon offset
{
    // Check if we have a non-transparent outline color
    if (where)
    {
        Color &color = where->lineColor;
        scale width = where->lineWidth;
        scale v = where->visibility * color.alpha;
        if (v > 0.0 && width > 0.0)
        {
            if (!where->hasMaterial)
                glColor4f(color.red, color.green, color.blue, v);
            return true;
        }
    }
    return false;
}

Vector3& Mesh::calculateNormal(const Point3& v1,const Point3& v2,const Point3& v3)
// ----------------------------------------------------------------------------
//    Compute normal of specified triangle
// ----------------------------------------------------------------------------
{
    Point3 a;
    Point3 b;
    Vector3 normal;

    a.x = v1.x - v2.x;
    a.y = v1.y - v2.y;
    a.z = v1.z - v2.z;

    b.x = v2.x - v3.x;
    b.y = v2.y - v3.y;
    b.z = v2.z - v3.z;

    normal.x = (a.y * b.z) - (a.z * b.y);
    normal.y = (a.z * b.x) - (a.x * b.z);
    normal.z = (a.x * b.y) - (a.y * b.x);

    return normal.Normalize();
}

Box3 Cube::Bounds(Layout *where)
// ----------------------------------------------------------------------------
//   Return the bounding box for a 3D shape
// ----------------------------------------------------------------------------
{
    return bounds + where->offset;
}

void Cube::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Draw the cube within the bounding box
// ----------------------------------------------------------------------------
{
    Box3 b = bounds + where->Offset();
    coord xl = bounds.lower.x;
    coord yl = bounds.lower.y;
    coord zl = bounds.lower.z;
    coord xu = bounds.upper.x;
    coord yu = bounds.upper.y;
    coord zu = bounds.upper.z;

    coord vertices[][3] =
    {
        {xl, yl, zl}, {xl, yu, zl}, {xu, yu, zl}, {xu, yl, zl},
        {xl, yl, zu}, {xu, yl, zu}, {xu, yu, zu}, {xl, yu, zu},
        {xl, yl, zl}, {xu, yl, zl}, {xu, yl, zu}, {xl, yl, zu},
        {xl, yu, zl}, {xl, yu, zu}, {xu, yu, zu}, {xu, yu, zl},
        {xl, yu, zl}, {xl, yl, zl}, {xl, yl, zu}, {xl, yu, zu},
        {xu, yl, zl}, {xu, yu, zl}, {xu, yu, zu}, {xu, yl, zu}
    };

    static GLdouble textures[][2] = {
        {1, 0}, {1, 1}, {0, 1}, {0, 0},
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
        {0, 1}, {0, 0}, {1, 0}, {1, 1},
        {0, 1}, {0, 0}, {1, 0}, {1, 1},
        {1, 0}, {1, 1}, {0, 1}, {0, 0}
    };

    static GLfloat normals[][3] =
    {
        { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1},
        { 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1},
        { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0},
        { 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0},
        {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0},
        { 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0},
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vertices);

    // Set normals only if we have lights or shaders
    if(where->currentLights || where->programId)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, normals);
    }

    //Active texture coordinates for all used units
    std::map<uint, TextureState>::iterator it;
    for(it = where->fillTextures.begin(); it != where->fillTextures.end(); it++)
        if(((*it).second).id)
            enableTexCoord((*it).first, textures);

    setTexture(where);

    if (setFillColor(where))
        glDrawArrays(GL_QUADS, 0, 24);
    if (setLineColor(where))
        for (uint face = 0; face < 6; face++)
            glDrawArrays(GL_LINE_LOOP, 4*face, 4);

    for(it = where->fillTextures.begin(); it != where->fillTextures.end(); it++)
        if(((*it).second).id)
            disableTexCoord((*it).first);

    if(where->currentLights || where->programId)
        glDisableClientState(GL_NORMAL_ARRAY);

    glDisableClientState(GL_VERTEX_ARRAY);
}

// ============================================================================
//
//    Mesh
//
// ============================================================================

void MeshBased::Draw(Mesh *mesh, Layout *where)
// ----------------------------------------------------------------------------
//    Draw the mesh within the bounding box
// ----------------------------------------------------------------------------
{
    Point3 p = bounds.Center() + where->Offset();

    // Optimize drawing of convex
    // shapes with backface culling
    // (doesn't need to draw back faces)
    glEnable(GL_CULL_FACE);

    glPushMatrix();
    glPushAttrib(GL_ENABLE_BIT);
    glTranslatef(p.x, p.y, p.z);
    glScalef(bounds.Width(), bounds.Height(), bounds.Depth());

    // Set Vertices
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, &mesh->vertices[0].x);

    // Set normals only if we have lights or shaders
    if(where->currentLights || where->programId)
    {
        glEnable(GL_NORMALIZE);
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_DOUBLE, 0, &mesh->normals[0].x);
    }

    //Active texture coordinates for all used units
    std::map<uint, TextureState>::iterator it;
    for(it = where->fillTextures.begin(); it != where->fillTextures.end(); it++)
        if(((*it).second).id)
            enableTexCoord((*it).first, &mesh->textures[0].x);

    // Apply textures
    setTexture(where);

    // Use painter algorithm to draw transparency
    // This was made necessary by Bug #1403.
    scale v = where->visibility * where->fillColor.alpha;
    if(v != 1.0)
    {
        glCullFace(GL_FRONT);
        glDepthMask(false);
        if (setFillColor(where))
            glDrawArrays(GL_QUAD_STRIP, 0, mesh->textures.size());
    }

    // Normal Drawing
    glCullFace(GL_BACK);
    if (setFillColor(where))
        glDrawArrays(GL_QUAD_STRIP, 0, mesh->textures.size());
    if (setLineColor(where))
        glDrawArrays(GL_LINE_LOOP, 0, mesh->textures.size());

    // Disable texture coordinates
    for(it = where->fillTextures.begin(); it != where->fillTextures.end(); it++)
        if(((*it).second).id)
            disableTexCoord((*it).first);

    // Disable normals
    if(where->currentLights || where->programId)
        glDisableClientState(GL_NORMAL_ARRAY);

    if(v != 1.0)
        glDepthMask(true);

    glDisableClientState(GL_VERTEX_ARRAY);

    glPopAttrib();
    glPopMatrix();

    glDisable(GL_CULL_FACE);
}

// ============================================================================
//
//    Sphere shape
//
// ============================================================================

SphereMesh::SphereMesh(uint slices, uint stacks)
// ----------------------------------------------------------------------------
//    Construct a unit sphere with given number of slices and stacks
// ----------------------------------------------------------------------------
{
    double radius = 0.5;
    for (uint j = 0; j < stacks; j++)
    {
        GLfloat phi      = M_PI * j / stacks;
        GLfloat incr_phi = M_PI * (j + 1) / stacks;

        // Compute phi components
        float sinPhi     =  sin(phi);
        float cosPhi     =  cos(phi);
        float sinIncrPhi =  sin(incr_phi);
        float cosIncrPhi =  cos(incr_phi);

        for (uint i = 0; i <= slices; i++) {
            GLfloat theta  = 2 * M_PI * i / slices;
            // Compute teta components (add an offset to be adaptated to the
            // old version)
            float sinTheta = sin(theta - M_PI/2);
            float cosTheta = cos(theta - M_PI/2) ;

            // First vertex
            textures.push_back(Point(1 - (double) i / slices,
                                     1 - (double) (j+1) / stacks));
            normals.push_back(Point3(cosTheta * sinIncrPhi, cosIncrPhi,
                                     sinTheta * sinIncrPhi));
            vertices.push_back(Point3(radius * cosTheta * sinIncrPhi,
                                      radius * cosIncrPhi,
                                      radius * sinTheta * sinIncrPhi));

            // Second vertex
            textures.push_back(Point(1 - (double) i / slices,
                                     1 - (double) j / stacks));
            normals.push_back(Point3(cosTheta * sinPhi, cosPhi,
                                     sinTheta * sinPhi));
            vertices.push_back(Point3(radius * cosTheta * sinPhi,
                                      radius * cosPhi,
                                      radius * sinTheta * sinPhi));
        }
    }
}

Sphere::SphereCache Sphere::cache;

void Sphere::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Draw the sphere within the bounding box
// ----------------------------------------------------------------------------
{
    Mesh * mesh = NULL;
    Key key(slices, stacks);
    SphereCache::iterator found = cache.find(key);
    if (found == cache.end())
    {
        // Prune the map if it gets too big
        while (cache.size() > MAX_SPHERES)
        {
            SphereCache::iterator first = cache.begin();
            delete (*first).second;
            cache.erase(first);
        }
        mesh = new SphereMesh(slices, stacks);
        cache[key] = mesh;
    }
    else
    {
        mesh = (*found).second;
    }

    MeshBased::Draw(mesh, where);
}

// ============================================================================
//
//    Torus shape
//
// ============================================================================

TorusMesh::TorusMesh(uint slices, uint stacks, double ratio)
// ----------------------------------------------------------------------------
//    Construct a unit torus with given number of slices, stacks and ratio
// ----------------------------------------------------------------------------
{
    double minRadius = ratio * 0.25;
    double majRadius = 0.25;
    double thickness = 0.25;

    for (uint j = 0; j < stacks; j++) {
        GLfloat phi      = 2 * M_PI * j / stacks;
        GLfloat incr_phi = 2 * M_PI * (j + 1) / stacks;

        // Compute phi components
        float sinPhi     =  sin(phi);
        float cosPhi     =  cos(phi);
        float sinIncrPhi =  sin(incr_phi);
        float cosIncrPhi =  cos(incr_phi);

        for (uint i = 0; i <= slices; i++) {
            GLfloat theta  = 2 * M_PI * i / slices;
            // Compute teta components
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // First vertex
            textures.push_back(Vector((double) i / slices, (double) (j+1) / stacks));
            normals.push_back(Vector3( sinTheta * cosIncrPhi, sinIncrPhi,  cosTheta * cosIncrPhi));
            vertices.push_back(Vector3((majRadius + minRadius * cosIncrPhi) * sinTheta,
                                       (thickness * sinIncrPhi),
                                       (majRadius + minRadius * cosIncrPhi) * cosTheta));

            // Second vertex
            textures.push_back(Vector((double) i / slices, (double) j / stacks));
            normals.push_back(Vector3(sinTheta * cosPhi, sinPhi, cosTheta * cosPhi));
            vertices.push_back(Vector3((majRadius + minRadius * cosPhi) * sinTheta,
                                       (thickness * sinPhi),
                                       (majRadius + minRadius * cosPhi) * cosTheta));
        }
    }
}

Torus::TorusCache Torus::cache;

void Torus::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Draw the torus within the bounding box
// ----------------------------------------------------------------------------
{
    Mesh * mesh = NULL;
    Key key(slices, stacks, ratio);
    TorusCache::iterator found = cache.find(key);
    if (found == cache.end())
    {
        // Prune the map if it gets too big
        while (cache.size() > MAX_TORUS)
        {
            TorusCache::iterator first = cache.begin();
            delete (*first).second;
            cache.erase(first);
        }
        mesh = new TorusMesh(slices, stacks, ratio);
        cache[key] = mesh;
    }
    else
    {
        mesh = (*found).second;
    }

    MeshBased::Draw(mesh, where);
}

// ============================================================================
//
//    Cone shape
//
// ============================================================================

ConeMesh::ConeMesh(double ratio)
// ----------------------------------------------------------------------------
//    Construct a (possibly truncated) unit cone - Limit case is a cylinder
// ----------------------------------------------------------------------------
{
    for (double a = 0; a <= 2 * M_PI; a += M_PI / 30)
    {
        double ca = cos(a);
        double sa = sin(a);

        double s = a / (2 * M_PI);
        textures.push_back(Point(s, 0));
        vertices.push_back(Point3(ca, sa, -0.5));

        textures.push_back(Point(s, 1));
        vertices.push_back(Point3(ca * ratio, sa * ratio, 0.5));
    }

    // Compute normal of each vertex according to those calculate for
    // neighbouring faces
    // NOTE: First and last normals are the same because of QUAD_STRIP
    Vector3 previousFaceNorm, nextFaceNorm;
    previousFaceNorm = calculateNormal(vertices[vertices.size() - 2],
                                       vertices[vertices.size() - 1],
                                       vertices[0]);
    nextFaceNorm = calculateNormal(vertices[0], vertices[1], vertices[2]);
    normals.push_back(((previousFaceNorm + nextFaceNorm)/2));
    normals.push_back(((previousFaceNorm + nextFaceNorm)/2));
    for(unsigned int i = 2; i < vertices.size() - 2; i +=2)
    {
        previousFaceNorm = nextFaceNorm;
        if(i < vertices.size() - 2)
            nextFaceNorm = calculateNormal(vertices[i],
                                           vertices[i + 1],
                                           vertices[i + 2]);
        else
            nextFaceNorm = calculateNormal(vertices[vertices.size() - 2],
                                           vertices[vertices.size() - 1],
                                           vertices[0]);

        normals.push_back(((previousFaceNorm + nextFaceNorm)/2));
        normals.push_back(((previousFaceNorm + nextFaceNorm)/2));
    }
    normals.push_back(normals[0]);
    normals.push_back(normals[0]);
}


Cone::ConeCache Cone::cache;

void Cone::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Draw the cone within the bounding box
// ----------------------------------------------------------------------------
{
    Mesh * mesh = NULL;
    Key key(ratio);
    ConeCache::iterator found = cache.find(key);
    if (found == cache.end())
    {
        // Prune the map if it gets too big
        while (cache.size() > MAX_CONES)
        {
            ConeCache::iterator first = cache.begin();
            delete (*first).second;
            cache.erase(first);
        }
        mesh = new ConeMesh(ratio);
        cache[key] = mesh;
    }
    else
    {
        mesh = (*found).second;
    }

    MeshBased::Draw(mesh, where);
}

TAO_END
