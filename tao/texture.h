#ifndef TEXTURE_H
#define TEXTURE_H
// ****************************************************************************
//  texture.h                                                       Tao project
// ****************************************************************************
//
//   File Description:
//
//     Loading a texture from an image
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

#include "tao.h"
#include "tree.h"
#include "widget.h"
#include "tao_gl.h"
#include "info_trash_can.h"
#include <QMovie>

TAO_BEGIN

struct ImageTextureInfo : XL::Info
// ----------------------------------------------------------------------------
//    Hold information about an image texture
// ----------------------------------------------------------------------------
{
    typedef ImageTextureInfo *data_t;
    struct  Texture { GLuint id, width, height; };
    typedef std::map<text, Texture> texture_map;
    enum { MAX_TEXTURES = 6000 };

    ImageTextureInfo();
    ~ImageTextureInfo();

    Texture load(text img, text docPath);
    operator data_t() { return this; }
    uint        width, height;

    static Texture &defaultTexture();
};


struct AnimatedTextureInfo : ImageTextureInfo
// ----------------------------------------------------------------------------
//    Hold information about an image texture
// ----------------------------------------------------------------------------
{
    typedef AnimatedTextureInfo *data_t;

    AnimatedTextureInfo();
    ~AnimatedTextureInfo();

    Texture load(text img);
    GLuint bind(text img);
    operator data_t() { return this; }

    static texture_map textures;

public:
    QMovie movie;
};

struct TextureIdInfo : XL::Info, InfoTrashCan
// ----------------------------------------------------------------------------
//   Info recording a given texture ID in a tree
// ----------------------------------------------------------------------------
{
    TextureIdInfo(): textureId(0)
    {
        GL.GenTextures(1, &textureId);
    }
    ~TextureIdInfo()
    {
        GL.DeleteTextures(1, &textureId);
    }
    virtual void Delete()
    {
        trash.push_back(this);
    }
    GLuint bind()
    {
        GL.BindTexture(GL_TEXTURE_2D, textureId);
        return textureId;
    }

    GLuint textureId;
};

TAO_END

#endif // texture.h

