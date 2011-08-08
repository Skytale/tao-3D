// ****************************************************************************
//  transforms.cpp                                                  Tao project
// ****************************************************************************
//
//   File Description:
//
//    Record transformations being applied in a layout
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

#include "transforms.h"
#include "layout.h"
#include "shapes3d.h"
#include "context.h"
#include "widget.h"


TAO_BEGIN

void Transform::Draw(Layout *where)
// ----------------------------------------------------------------------------
//   Default draws a cube
// ----------------------------------------------------------------------------
{
    Color line(1.0, 0.0, 0.0, 0.5);
    Color fill(0.0, 0.7, 1.0, 0.5);
    XL::Save<Color> saveLine(where->lineColor, line);
    XL::Save<Color> saveFill(where->fillColor, fill);
    Box3 cubeBox(-25, -25, -25, 50, 50, 50);
    Cube cube(cubeBox);
    cube.Draw(where);
}


void Transform::DrawSelection(Layout *where)
// ----------------------------------------------------------------------------
//   Transforms don't have a selection, they just draw themselves
// ----------------------------------------------------------------------------
{
    Draw(where);
}


void Transform::Identify(Layout *where)
// ----------------------------------------------------------------------------
//   Transforms don't have their own identity, they just draw themselves
// ----------------------------------------------------------------------------
{
    Draw(where);
}


void ResetTransform::Draw(Layout *where)
// ----------------------------------------------------------------------------
//   Reset the transformation matrix and other parameters
// ----------------------------------------------------------------------------
{
    Widget *widget = where->Display();
    widget->resetModelviewMatrix();
    where->hasPixelBlur = false;
    where->has3D = false;
}

void printMatrix(GLint model = -1);
void Rotation::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Rotation in a drawing
// ----------------------------------------------------------------------------
{
    // BUG? fmod required to avoid incorrect rotations with large values
    // (>1290000000)
    amount = fmod(amount, 360.0);
    std::cerr << "Rotation::Draw on "<< where << " with amount % 360 "<< amount << std::endl;
    printMatrix();
    glRotated(amount, xaxis, yaxis, zaxis);
    double amod90 = fmod(amount, 90.0);
    if (amod90 < -0.01 || amod90 > 0.01)
        where->hasPixelBlur = true;
    if (amount != 0.0)
    {
        if (xaxis != 0.0 || yaxis != 0.0)
            where->has3D = true;
        else if (zaxis > 0)
            where->planarRotation += amount;
        else if (zaxis < 0)
            where->planarRotation -= amount;
    }
    where->offset = Point3();
}


void Translation::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Rotation in a drawing
// ----------------------------------------------------------------------------
{
    std::cerr << "Translate::Draw on " << where << " xaxis is " << xaxis <<std::endl;
    printMatrix();
    glTranslatef(xaxis, yaxis, zaxis);
    printMatrix();
    if (zaxis != 0.0)
        where->hasPixelBlur = true;
    where->offset = Point3();
}


void Scale::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Scale in a drawing
// ----------------------------------------------------------------------------
{
    glScalef(xaxis, yaxis, zaxis);
    if (xaxis != 1.0 || yaxis != 1.0)
        where->hasPixelBlur = true;
    if (xaxis == yaxis && xaxis == zaxis)
        where->planarScale *= xaxis;
    else
        where->has3D = true;
    where->offset = Point3();
}


void MoveTo::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Position in a drawing
// ----------------------------------------------------------------------------
{
    where->offset = Point3(xaxis, yaxis, zaxis);
}


void MoveToRel::Draw(Layout *where)
// ----------------------------------------------------------------------------
//    Position in a drawing
// ----------------------------------------------------------------------------
{
    where->offset += Vector3(xaxis, yaxis, zaxis);
}

void printMatrix(GLint model)
{
    GLdouble matrix[16];
    GLint cur = 0;
    glGetIntegerv(GL_MATRIX_MODE, &cur);
    std::cerr << "Current matrix is " << cur <<std::endl;
    if (model != -1 && model != cur)
    {
        glMatrixMode(model);
        cur = model;
        std::cerr << "Matrix mode set to " << model <<std::endl;
    }
    glGetDoublev(cur, matrix);
    for (int i = 0; i < 16; i+=4)
    {
        std::cerr << matrix[i] << "  " << matrix[i+1] << "  " << matrix[i+2]
                << "  " <<matrix[i+3] << "  " <<std::endl;
    }
}

TAO_END
