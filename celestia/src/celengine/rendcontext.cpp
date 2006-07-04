// rendcontext.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
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
    GL_POINTS
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
    renderPass(PrimaryPass)
{
}


RenderContext::RenderContext(const Mesh::Material* _material)
{
    if (_material == NULL)
        material = &defaultMaterial;
    else
        material = _material;
}


static void setVertexArrays(const Mesh::VertexDescription& desc)
{
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
RenderContext::drawGroup(const Mesh::PrimitiveGroup& group)
{
    // Skip rendering if this is the emissive pass but there's no
    // emissive texture.
    if (renderPass == EmissivePass &&
        material->maps[Mesh::EmissiveMap] == InvalidResource)
    {
        return;
    }

    glDrawElements(GLPrimitiveModes[(int) group.prim],
                   group.nIndices,
                   GL_UNSIGNED_INT,
                   group.indices);
}


FixedFunctionRenderContext::FixedFunctionRenderContext() :
    RenderContext(),
    blendOn(false),
    specularOn(false)
{
}


FixedFunctionRenderContext::FixedFunctionRenderContext(const Mesh::Material* _material) :
    RenderContext(_material),
    blendOn(false),
    specularOn(false)
{
}


void
FixedFunctionRenderContext::makeCurrent(const Mesh::Material& m)
{
    if (getRenderPass() == PrimaryPass)
    {
        Texture* t = NULL;
        if (m.maps[Mesh::DiffuseMap] != InvalidResource)
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

        glColor4f(m.diffuse.red(),
                  m.diffuse.green(),
                  m.diffuse.blue(),
                  m.opacity);

        bool blendOnNow = false;
        if (m.opacity != 1.0f || (t != NULL && t->hasAlpha()))
            blendOnNow = true;

        if (blendOnNow != blendOn)
        {
            blendOn = blendOnNow;
            if (blendOn)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                //glEnable(GL_ALPHA_TEST);
                //glAlphaFunc(GL_GEQUAL, 0.01f);
            }
            else
            {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                //glDisable(GL_ALPHA_TEST);
            }
        }

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
    else if (getRenderPass() == EmissivePass)
    {
        Texture* t = NULL;
        if (m.maps[Mesh::EmissiveMap] != InvalidResource)
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
    setStandardVertexArrays(desc, vertexData);
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
        glEnableClientState(GL_NORMAL_ARRAY);
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
}


GLSL_RenderContext::GLSL_RenderContext(const LightingState& ls, float _objRadius, const Mat4f& _xform) :
    lightingState(ls),
    blendOn(false),
    objRadius(_objRadius),
    xform(_xform)
{
    initLightingEnvironment();
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
    
    clog << "totalShadows: " << totalShadows << '\n';
}
  

void
GLSL_RenderContext::setLightingParameters(CelestiaGLProgram& prog, Color materialDiffuse, Color materialSpecular)
{
    unsigned int nLights = min(MaxShaderLights, lightingState.nLights);

    Vec3f diffuseColor(materialDiffuse.red(),
                       materialDiffuse.green(),
                       materialDiffuse.blue());
    Vec3f specularColor(materialSpecular.red(),
                        materialSpecular.green(),
                        materialSpecular.blue());
    
    for (unsigned int i = 0; i < nLights; i++)
    {
        const DirectionalLight& light = lightingState.lights[i];

        Vec3f lightColor = Vec3f(light.color.red(),
                                 light.color.green(),
                                 light.color.blue()) * light.irradiance;
        prog.lights[i].direction = light.direction_obj;

        if (shaderProps.usesShadows() ||
            shaderProps.usesFragmentLighting() ||
            shaderProps.lightModel == ShaderProperties::RingIllumModel)
        {
            prog.fragLightColor[i] = Vec3f(lightColor.x * diffuseColor.x,
                                           lightColor.y * diffuseColor.y,
                                           lightColor.z * diffuseColor.z);
            if (shaderProps.lightModel == ShaderProperties::SpecularModel)
            {
                prog.fragLightSpecColor[i] = Vec3f(lightColor.x * specularColor.x,
                                                   lightColor.y * specularColor.y,
                                                   lightColor.z * specularColor.z);
            }
        }
        else
        {
            prog.lights[i].diffuse = Vec3f(lightColor.x * diffuseColor.x,
                                            lightColor.y * diffuseColor.y,
                                            lightColor.z * diffuseColor.z);
        }
        
        prog.lights[i].specular = Vec3f(lightColor.x * specularColor.x,
                                         lightColor.y * specularColor.y,
                                         lightColor.z * specularColor.z);

        Vec3f halfAngle_obj = lightingState.eyeDir_obj + light.direction_obj;
        if (halfAngle_obj.length() != 0.0f)
            halfAngle_obj.normalize();
        prog.lights[i].halfVector = halfAngle_obj;
    }
    
    prog.eyePosition = lightingState.eyePos_obj;
    prog.ambientColor = lightingState.ambientColor;
}


void
GLSL_RenderContext::setShadowParameters(CelestiaGLProgram& prog)
{
    // TODO: this code is largely a copy of some code in render.cpp; we should
    // have just a single instance of the code.
    for (unsigned int li = 0;
         li < min(lightingState.nLights, MaxShaderLights);
         li++)
    {
        vector<EclipseShadow>* shadows = lightingState.shadows[li];

        if (shadows != NULL)
        {
            unsigned int nShadows = min((size_t) MaxShaderShadows, 
                                        shadows->size());

            for (unsigned int i = 0; i < nShadows; i++)
            {
                EclipseShadow& shadow = shadows->at(i);
                CelestiaGLProgramShadow& shadowParams = prog.shadows[li][i];

                float R2 = 0.25f;
                float umbra = shadow.umbraRadius / shadow.penumbraRadius;
                umbra = umbra * umbra;
                if (umbra < 0.0001f)
                    umbra = 0.0001f;
                else if (umbra > 0.99f)
                    umbra = 0.99f;

                float umbraRadius = R2 * umbra;
                float penumbraRadius = R2;
                float shadowBias = 1.0f / (1.0f - penumbraRadius / umbraRadius);
                shadowParams.bias = shadowBias;
                shadowParams.scale = -shadowBias / umbraRadius;

                // Compute the transformation to use for generating texture
                // coordinates from the object vertices.
                Point3f origin = shadow.origin * xform;
                Vec3f dir = shadow.direction * xform;
                float scale = objRadius / shadow.penumbraRadius;
                Vec3f axis = Vec3f(0, 1, 0) ^ dir;
                float angle = (float) acos(Vec3f(0, 1, 0) * dir);
                axis.normalize();
                Mat4f mat = Mat4f::rotation(axis, -angle);
                Vec3f sAxis = Vec3f(0.5f * scale, 0, 0) * mat;
                Vec3f tAxis = Vec3f(0, 0, 0.5f * scale) * mat;

                float sw = (Point3f(0, 0, 0) - origin) * sAxis / objRadius + 0.5f;
                float tw = (Point3f(0, 0, 0) - origin) * tAxis / objRadius + 0.5f;
                shadowParams.texGenS = Vec4f(sAxis.x, sAxis.y, sAxis.z, sw);
                shadowParams.texGenT = Vec4f(tAxis.x, tAxis.y, tAxis.z, tw);
            }
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
    shaderProps.lightModel = ShaderProperties::DiffuseModel;
    
    if (m.maps[Mesh::DiffuseMap] != InvalidResource)
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
            textures[nTextures++] = bumpTex;
        }
    }

    if (m.specular != Color::Black)
    {
        shaderProps.lightModel = ShaderProperties::SpecularModel;
        specTex = GetTextureManager()->find(m.maps[Mesh::SpecularMap]);
        if (specTex == NULL)
        {
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
            shaderProps.texUsage |= ShaderProperties::NightTexture;
            textures[nTextures++] = emissiveTex;
        }
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
    
    setLightingParameters(*prog, m.diffuse, m.specular);
    if (shaderProps.shadowCounts != 0)    
        setShadowParameters(*prog);

    prog->shininess = m.specularPower;
    //prog->ambientColor = Vec3f(ri.ambientColor.red(), ri.ambientColor.green(), ri.ambientColor.blue());
    // TODO: handle emissive color
    
    if (emissiveTex != NULL)
    {
        prog->nightTexMin = 1.0f;
    }
        
    bool blendOnNow = false;
    if (m.opacity != 1.0f || (baseTex != NULL && baseTex->hasAlpha()))
        blendOnNow = true;

    if (blendOnNow != blendOn)
    {
        blendOn = blendOnNow;
        if (blendOn)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
    }
}


void
GLSL_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc,
                                     void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}
