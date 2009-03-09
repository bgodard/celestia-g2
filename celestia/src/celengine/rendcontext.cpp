// rendcontext.cpp
//
// Copyright (C) 2004-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>
#include "rendcontext.h"
#include "texmanager.h"
#include "gl.h"
#include "glext.h"
#include "vecgl.h"

using namespace std;


static Mesh::Material defaultMaterial;

static GLenum GLPrimitiveModes[Mesh::PrimitiveTypeMax] =
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS,
    GL_POINTS,
};

static GLenum GLComponentTypes[Mesh::FormatMax] =
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

static int GLComponentCounts[Mesh::FormatMax] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};


enum {
    TangentAttributeIndex = 6,
    PointSizeAttributeIndex = 7,
};


static void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        void* vertexData);
static void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData);


RenderContext::RenderContext() :
    material(&defaultMaterial),
    locked(false),
    renderPass(PrimaryPass),
    pointScale(1.0f),
    usePointSize(false),
    useNormals(true),
    useColors(false),
    useTexCoords(true)
{
}


RenderContext::RenderContext(const Mesh::Material* _material)
{
    if (_material == NULL)
        material = &defaultMaterial;
    else
        material = _material;
}


const Mesh::Material*
RenderContext::getMaterial() const
{
    return material;
}


void
RenderContext::setMaterial(const Mesh::Material* newMaterial)
{
    if (!locked)
    {
        if (newMaterial == NULL)
            newMaterial = &defaultMaterial;

        if (renderPass == PrimaryPass)
        {
            if (newMaterial != material)
            {
                material = newMaterial;
                makeCurrent(*material);
            }
        }
        else if (renderPass == EmissivePass)
        {
            if (material->maps[Mesh::EmissiveMap] !=
                newMaterial->maps[Mesh::EmissiveMap])
            {
                material = newMaterial;
                makeCurrent(*material);
            }
        }
    }
}


void
RenderContext::setPointScale(float _pointScale)
{
    pointScale = _pointScale;
}


float
RenderContext::getPointScale() const
{
    return pointScale;
}


void
RenderContext::setCameraOrientation(const Quatf& q)
{
    cameraOrientation = q;
}


Quatf
RenderContext::getCameraOrientation() const
{
    return cameraOrientation;
}


void
RenderContext::drawGroup(const Mesh::PrimitiveGroup& group)
{
    // Skip rendering if this is the emissive pass but there's no
    // emissive texture.
    if (renderPass == EmissivePass &&
        material->maps[Mesh::EmissiveMap] == InvalidResource)
    {
        return;
    }

    if (group.prim == Mesh::SpriteList)
    {
        glEnable(GL_POINT_SPRITE_ARB);
        glx::glActiveTextureARB(GL_TEXTURE0_ARB);
        glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    }

    glDrawElements(GLPrimitiveModes[(int) group.prim],
                   group.nIndices,
                   GL_UNSIGNED_INT,
                   group.indices);

    if (group.prim == Mesh::SpriteList)
    {
        glDisable(GL_POINT_SPRITE_ARB);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    }
}


FixedFunctionRenderContext::FixedFunctionRenderContext() :
    RenderContext(),
    blendMode(Mesh::InvalidBlend),
    specularOn(false),
    lightingEnabled(true)
{
}


FixedFunctionRenderContext::FixedFunctionRenderContext(const Mesh::Material* _material) :
    RenderContext(_material),
    blendMode(Mesh::InvalidBlend),
    specularOn(false),
    lightingEnabled(true)
{
}


FixedFunctionRenderContext::~FixedFunctionRenderContext()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


static int blah = 0;
void
FixedFunctionRenderContext::makeCurrent(const Mesh::Material& m)
{
    if (getRenderPass() == PrimaryPass)
    {
        Texture* t = NULL;
        if (m.maps[Mesh::DiffuseMap] != InvalidResource && useTexCoords)
            t = GetTextureManager()->find(m.maps[Mesh::DiffuseMap]);

        if (t == NULL)
        {
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            t->bind();
        }

        Mesh::BlendMode newBlendMode = Mesh::InvalidBlend;
        if (m.opacity != 1.0f ||
            m.blend == Mesh::AdditiveBlend ||
            (t != NULL && t->hasAlpha()))
        {
            newBlendMode = m.blend;
        }

        if (newBlendMode != blendMode)
        {
            blendMode = newBlendMode;
            switch (blendMode)
            {
            case Mesh::NormalBlend:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                break;
            case Mesh::AdditiveBlend:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDepthMask(GL_FALSE);
                break;
            case Mesh::PremultipliedAlphaBlend:
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                break;
            default:
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                break;
            }
        }

        if (useNormals || !lightingEnabled)
        {
#ifdef HDR_COMPRESS
            glColor4f(m.diffuse.red()   * 0.5f,
                      m.diffuse.green() * 0.5f,
                      m.diffuse.blue()  * 0.5f,
                      m.opacity);
#else
            glColor4f(m.diffuse.red(),
                      m.diffuse.green(),
                      m.diffuse.blue(),
                      m.opacity);
#endif

            if (m.specular == Color::Black)
            {
                float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                float zero = 0.0f;
                glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
                glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
                specularOn = false;
            }
            else
            {
                float matSpecular[4] = { m.specular.red(),
                                         m.specular.green(),
                                         m.specular.blue(),
                                         0.0f };
                glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
                glMaterialfv(GL_FRONT, GL_SHININESS, &m.specularPower);
                specularOn = true;
            }
            
            {
                float matEmissive[4] = { m.emissive.red(),
                                         m.emissive.green(),
                                         m.emissive.blue(),
                                         0.0f };
                glMaterialfv(GL_FRONT, GL_EMISSION, matEmissive);
            }
        }
        else
        {
            // When lighting without normals, we'll just merge everything
            // into the emissive color. This makes normal-less lighting work
            // more like it does in the GLSL path, though it's not very
            // useful without shadows.
            float matBlack[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float zero = 0.0f;
            glMaterialfv(GL_FRONT, GL_DIFFUSE, matBlack);
            glMaterialfv(GL_FRONT, GL_SPECULAR, matBlack);
            glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
            {
#ifdef HDR_COMPRESS
                float matEmissive[4] = { m.emissive.red() + m.diffuse.red() * 0.5f,
                                         m.emissive.green() + m.diffuse.green() * 0.5f,
                                         m.emissive.blue() + m.diffuse.blue() * 0.5f,
                                         m.opacity };
#else
                float matEmissive[4] = { m.emissive.red() + m.diffuse.red(),
                                         m.emissive.green() + m.diffuse.green(),
                                         m.emissive.blue() + m.diffuse.blue(),
                                         m.opacity };
#endif
                glMaterialfv(GL_FRONT, GL_EMISSION, matEmissive);
            }
        }
    }
    else if (getRenderPass() == EmissivePass)
    {
        Texture* t = NULL;
        if (m.maps[Mesh::EmissiveMap] != InvalidResource && useTexCoords)
            t = GetTextureManager()->find(m.maps[Mesh::EmissiveMap]);

        if (t == NULL)
        {
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            t->bind();
        }
    }
}


void
FixedFunctionRenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    // Update the material if normals appear or disappear in the vertex
    // description.
    bool useNormalsNow = (desc.getAttribute(Mesh::Normal).format == Mesh::Float3);
    bool useColorsNow = (desc.getAttribute(Mesh::Color0).format != Mesh::InvalidFormat);
    bool useTexCoordsNow = (desc.getAttribute(Mesh::Texture0).format != Mesh::InvalidFormat);

    if (useNormalsNow != useNormals ||
        useColorsNow != useColors ||
        useTexCoordsNow != useTexCoords)
    {
        useNormals = useNormalsNow;
        useColors = useColorsNow;
        useTexCoords = useTexCoordsNow;
        if (getMaterial() != NULL)
            makeCurrent(*getMaterial());
    }

    setStandardVertexArrays(desc, vertexData);
}


void
FixedFunctionRenderContext::setLighting(bool enabled)
{
    lightingEnabled = enabled;
    if (lightingEnabled)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);
}


void
VP_Combiner_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}


void
VP_FP_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}




void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        void* vertexData)
{
    const Mesh::VertexAttribute& position  = desc.getAttribute(Mesh::Position);
    const Mesh::VertexAttribute& normal    = desc.getAttribute(Mesh::Normal);
    const Mesh::VertexAttribute& color0    = desc.getAttribute(Mesh::Color0);
    const Mesh::VertexAttribute& texCoord0 = desc.getAttribute(Mesh::Texture0);

    // Can't render anything unless we have positions
    if (position.format != Mesh::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.stride,
                    reinterpret_cast<char*>(vertexData) + position.offset);

    // Set up the normal array
    switch (normal.format)
    {
    case Mesh::Float3:
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GLComponentTypes[(int) normal.format],
                        desc.stride,
                        reinterpret_cast<char*>(vertexData) + normal.offset);
        break;
    default:
        glDisableClientState(GL_NORMAL_ARRAY);
        break;
    }

    // Set up the color array
    switch (color0.format)
    {
    case Mesh::Float3:
    case Mesh::Float4:
    case Mesh::UByte4:
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GLComponentCounts[color0.format],
                       GLComponentTypes[color0.format],
                       desc.stride,
                       reinterpret_cast<char*>(vertexData) + color0.offset);
        break;
    default:
        glDisableClientState(GL_COLOR_ARRAY);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case Mesh::Float1:
    case Mesh::Float2:
    case Mesh::Float3:
    case Mesh::Float4:
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(GLComponentCounts[(int) texCoord0.format],
                          GLComponentTypes[(int) texCoord0.format],
                          desc.stride,
                          reinterpret_cast<char*>(vertexData) + texCoord0.offset);
        break;
    default:
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        break;
    }
}


void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData)
{
    const Mesh::VertexAttribute& tangent  = desc.getAttribute(Mesh::Tangent);
    const char* vertices = reinterpret_cast<const char*>(vertexData);

    switch (tangent.format)
    {
    case Mesh::Float3:
        glx::glEnableVertexAttribArrayARB(TangentAttributeIndex);
        glx::glVertexAttribPointerARB(TangentAttributeIndex,
                                      GLComponentCounts[(int) tangent.format],
                                      GLComponentTypes[(int) tangent.format],
                                      GL_FALSE,
                                      desc.stride,
                                      vertices + tangent.offset);
        break;
    default:
        glx::glDisableVertexAttribArrayARB(TangentAttributeIndex);
        break;
    }

    const Mesh::VertexAttribute& pointsize = desc.getAttribute(Mesh::PointSize);
    switch (pointsize.format)
    {
    case Mesh::Float1:
        glx::glEnableVertexAttribArrayARB(PointSizeAttributeIndex);
        glx::glVertexAttribPointerARB(PointSizeAttributeIndex,
                                      GLComponentCounts[(int) pointsize.format],
                                      GLComponentTypes[(int) pointsize.format],
                                      GL_FALSE,
                                      desc.stride,
                                      vertices + pointsize.offset);
        break;
    default:
        glx::glDisableVertexAttribArrayARB(PointSizeAttributeIndex);
        break;
    }
}


/***** GLSL render context ******/

GLSL_RenderContext::GLSL_RenderContext(const LightingState& ls, float _objRadius, const Mat4f& _xform) :
    lightingState(ls),
    atmosphere(NULL),
    blendMode(Mesh::InvalidBlend),
    objRadius(_objRadius),
    xform(_xform),
    lunarLambert(0.0f)
{
    initLightingEnvironment();
}


GLSL_RenderContext::~GLSL_RenderContext()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glx::glDisableVertexAttribArrayARB(TangentAttributeIndex);
    glx::glDisableVertexAttribArrayARB(PointSizeAttributeIndex);
}


void
GLSL_RenderContext::initLightingEnvironment()
{
    // Set the light and shadow environment, which is constant for the entire model.
    // The material properties will be set per mesh.
    shaderProps.nLights = min(lightingState.nLights, MaxShaderLights);

    // Set the shadow information.
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;
    for (unsigned int li = 0; li < lightingState.nLights; li++)
    {
        if (lightingState.shadows[li] && !lightingState.shadows[li]->empty())
        {
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderShadows, lightingState.shadows[li]->size());
            shaderProps.setShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
        }
    }

}

void
GLSL_RenderContext::makeCurrent(const Mesh::Material& m)
{
    Texture* textures[4] = { NULL, NULL, NULL, NULL };
    unsigned int nTextures = 0;

    // Set up the textures used by this object
    Texture* baseTex = NULL;
    Texture* bumpTex = NULL;
    Texture* specTex = NULL;
    Texture* emissiveTex = NULL;

    shaderProps.texUsage = ShaderProperties::SharedTextureCoords;

    if (useNormals)
    {
        if (lunarLambert == 0.0f)
            shaderProps.lightModel = ShaderProperties::DiffuseModel;
        else
            shaderProps.lightModel = ShaderProperties::LunarLambertModel;
    }
    else
    {
        // "particle" lighting is the only type that doesn't
        // depend on having a surface normal.
        // Enable alternate particle model when vertex colors are present;
        // eventually, a render context method will enable the particle
        // model.
        if (useColors)
            shaderProps.lightModel = ShaderProperties::ParticleModel;
        else
            shaderProps.lightModel = ShaderProperties::ParticleDiffuseModel;
    }

    if (m.maps[Mesh::DiffuseMap] != InvalidResource && useTexCoords)
    {
        baseTex = GetTextureManager()->find(m.maps[Mesh::DiffuseMap]);
        if (baseTex != NULL)
        {
            shaderProps.texUsage |= ShaderProperties::DiffuseTexture;
            textures[nTextures++] = baseTex;
        }
    }

    if (m.maps[Mesh::NormalMap] != InvalidResource)
    {
        bumpTex = GetTextureManager()->find(m.maps[Mesh::NormalMap]);
        if (bumpTex != NULL)
        {
            shaderProps.texUsage |= ShaderProperties::NormalTexture;
            if (bumpTex->getFormatOptions() & Texture::DXT5NormalMap)
            {
                shaderProps.texUsage |= ShaderProperties::CompressedNormalTexture;
            }
            textures[nTextures++] = bumpTex;
        }
    }

    if (m.specular != Color::Black && useNormals)
    {
        shaderProps.lightModel = ShaderProperties::PerPixelSpecularModel;
        specTex = GetTextureManager()->find(m.maps[Mesh::SpecularMap]);
        if (specTex == NULL)
        {
            if (baseTex != NULL)
                shaderProps.texUsage |= ShaderProperties::SpecularInDiffuseAlpha;
        }
        else
        {
            shaderProps.texUsage |= ShaderProperties::SpecularTexture;
            textures[nTextures++] = specTex;
        }
    }

    if (m.maps[Mesh::EmissiveMap] != InvalidResource)
    {
        emissiveTex = GetTextureManager()->find(m.maps[Mesh::EmissiveMap]);
        if (emissiveTex != NULL)
        {
            shaderProps.texUsage |= ShaderProperties::EmissiveTexture;
            textures[nTextures++] = emissiveTex;
        }
    }

    if (usePointSize)
        shaderProps.texUsage |= ShaderProperties::PointSprite;
    if (useColors)
        shaderProps.texUsage |= ShaderProperties::VertexColors;

    if (atmosphere != NULL)
    {
        // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
        if (atmosphere->mieScaleHeight > 0.0f)
            shaderProps.texUsage |= ShaderProperties::Scattering;
    }

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = GetShaderManager().getShader(shaderProps);
    if (prog == NULL)
        return;

    prog->use();

    for (unsigned int i = 0; i < nTextures; i++)
    {
        glx::glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glEnable(GL_TEXTURE_2D);
        textures[i]->bind();
    }

    // setLightParameters() expects opacity in the alpha channel of the diffuse color
#ifdef HDR_COMPRESS
    Color diffuse(m.diffuse.red() * 0.5f, m.diffuse.green() * 0.5f, m.diffuse.blue() * 0.5f, m.opacity);
#else
    Color diffuse(m.diffuse.red(), m.diffuse.green(), m.diffuse.blue(), m.opacity);
#endif

    prog->setLightParameters(lightingState, diffuse, m.specular, m.emissive);

    if (shaderProps.shadowCounts != 0)
        prog->setEclipseShadowParameters(lightingState, objRadius, xform);

    // TODO: handle emissive color
    prog->shininess = m.specularPower;
    if (shaderProps.lightModel == ShaderProperties::LunarLambertModel)
    {
        prog->lunarLambert = lunarLambert;
    }

    // Generally, we want to disable depth writes for blend because it
    // makes translucent objects look a bit better (though there are
    // still problems when rendering them without sorting.) However,
    // when scattering atmospheres are enabled, we need to render with
    // depth writes on, otherwise the atmosphere will be drawn over
    // a planet mesh. See SourceForge bug #1855894 for more details.
    bool disableDepthWriteOnBlend = true;

    if (shaderProps.hasScattering())
    {
        prog->setAtmosphereParameters(*atmosphere, objRadius, objRadius);
        disableDepthWriteOnBlend = false;
    }

    if ((shaderProps.texUsage & ShaderProperties::PointSprite) != 0)
    {
        prog->pointScale = getPointScale();
    }

    Mesh::BlendMode newBlendMode = Mesh::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == Mesh::AdditiveBlend ||
        (baseTex != NULL && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        switch (blendMode)
        {
        case Mesh::NormalBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;
        case Mesh::AdditiveBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;
        case Mesh::PremultipliedAlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;                
        default:
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            break;
        }
    }
}


void
GLSL_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc,
                                    void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);

    // Normally, the shader that will be used depends only on the material.
    // But the presence of point size and normals can also affect the
    // shader, so force an update of the material if those attributes appear
    // or disappear in the new set of vertex arrays.
    bool usePointSizeNow = (desc.getAttribute(Mesh::PointSize).format == Mesh::Float1);
    bool useNormalsNow = (desc.getAttribute(Mesh::Normal).format == Mesh::Float3);
    bool useColorsNow = (desc.getAttribute(Mesh::Color0).format != Mesh::InvalidFormat);
    bool useTexCoordsNow = (desc.getAttribute(Mesh::Texture0).format != Mesh::InvalidFormat);

    if (usePointSizeNow != usePointSize ||
        useNormalsNow   != useNormals   ||
        useColorsNow    != useColors    ||
        useTexCoordsNow != useTexCoords)
    {
        usePointSize = usePointSizeNow;
        useNormals = useNormalsNow;
        useColors = useColorsNow;
        useTexCoords = useTexCoordsNow;
        if (getMaterial() != NULL)
            makeCurrent(*getMaterial());
    }
}


void
GLSL_RenderContext::setAtmosphere(const Atmosphere* _atmosphere)
{
    atmosphere = _atmosphere;
}

// Extended material properties -- currently just lunarLambert term
void
GLSL_RenderContext::setLunarLambert(float l)
{
    lunarLambert = l;
}


/***** GLSL-Unlit render context ******/

GLSLUnlit_RenderContext::GLSLUnlit_RenderContext(float _objRadius) :
    blendMode(Mesh::InvalidBlend),
    objRadius(_objRadius)
{
    initLightingEnvironment();
}


GLSLUnlit_RenderContext::~GLSLUnlit_RenderContext()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glx::glDisableVertexAttribArrayARB(TangentAttributeIndex);
    glx::glDisableVertexAttribArrayARB(PointSizeAttributeIndex);
}


void
GLSLUnlit_RenderContext::initLightingEnvironment()
{
    // Set the light and shadow environment, which is constant for the entire model.
    // The material properties will be set per mesh.
    shaderProps.nLights = 1;
}


void
GLSLUnlit_RenderContext::makeCurrent(const Mesh::Material& m)
{
    Texture* textures[4] = { NULL, NULL, NULL, NULL };
    unsigned int nTextures = 0;

    // Set up the textures used by this object
    Texture* baseTex = NULL;

    shaderProps.lightModel = ShaderProperties::EmissiveModel;
    shaderProps.texUsage = ShaderProperties::SharedTextureCoords;

    if (m.maps[Mesh::DiffuseMap] != InvalidResource && useTexCoords)
    {
        baseTex = GetTextureManager()->find(m.maps[Mesh::DiffuseMap]);
        if (baseTex != NULL)
        {
            shaderProps.texUsage |= ShaderProperties::DiffuseTexture;
            textures[nTextures++] = baseTex;
        }
    }

    if (usePointSize)
        shaderProps.texUsage |= ShaderProperties::PointSprite;
    if (useColors)
        shaderProps.texUsage |= ShaderProperties::VertexColors;

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = GetShaderManager().getShader(shaderProps);
    if (prog == NULL)
        return;

    prog->use();

    for (unsigned int i = 0; i < nTextures; i++)
    {
        glx::glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glEnable(GL_TEXTURE_2D);
        textures[i]->bind();
    }

#ifdef HDR_COMPRESS
    prog->lights[0].diffuse = Vec3f(m.diffuse.red()   * 0.5f,
                                    m.diffuse.green() * 0.5f,
                                    m.diffuse.blue()  * 0.5f);
#else
    prog->lights[0].diffuse = Vec3f(m.diffuse.red(),
                                    m.diffuse.green(),
                                    m.diffuse.blue());
#endif
    prog->opacity = m.opacity;

    if ((shaderProps.texUsage & ShaderProperties::PointSprite) != 0)
    {
        prog->pointScale = getPointScale();
    }

    Mesh::BlendMode newBlendMode = Mesh::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == Mesh::AdditiveBlend ||
        (baseTex != NULL && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        switch (blendMode)
        {
        case Mesh::NormalBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            break;
        case Mesh::AdditiveBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            break;
        case Mesh::PremultipliedAlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            break;                
        default:
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            break;
        }
    }
}


void
GLSLUnlit_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc,
                                         void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);

    // Normally, the shader that will be used depends only on the material.
    // But the presence of point size and normals can also affect the
    // shader, so force an update of the material if those attributes appear
    // or disappear in the new set of vertex arrays.
    bool usePointSizeNow = (desc.getAttribute(Mesh::PointSize).format == Mesh::Float1);
    bool useNormalsNow = (desc.getAttribute(Mesh::Normal).format == Mesh::Float3);
    bool useColorsNow = (desc.getAttribute(Mesh::Color0).format != Mesh::InvalidFormat);
    bool useTexCoordsNow = (desc.getAttribute(Mesh::Texture0).format != Mesh::InvalidFormat);

    if (usePointSizeNow != usePointSize ||
        useNormalsNow   != useNormals   ||
        useColorsNow    != useColors    ||
        useTexCoordsNow != useTexCoords)
    {
        usePointSize = usePointSizeNow;
        useNormals = useNormalsNow;
        useColors = useColorsNow;
        useTexCoords = useTexCoordsNow;
        if (getMaterial() != NULL)
            makeCurrent(*getMaterial());
    }
}
