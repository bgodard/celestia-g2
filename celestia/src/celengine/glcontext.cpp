// glcontext.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celutil/debug.h>
#include "gl.h"
#include "glext.h"
#include "glcontext.h"

using namespace std;


static VertexProcessor* vpNV = NULL;
static VertexProcessor* vpARB = NULL;


GLContext::GLContext() :
    renderPath(GLPath_Basic),
    vertexPath(VPath_Basic),
    vertexProc(NULL),
    maxSimultaneousTextures(1)
{
}

GLContext::~GLContext()
{
}


void GLContext::init(const vector<string>& ignoreExt)
{
    char* extensionsString = (char*) glGetString(GL_EXTENSIONS);
    if (extensionsString != NULL)
    {
        char* next = extensionsString;
        
        while (*next != '\0')
        {
            while (*next != '\0' && *next != ' ')
                next++;

            string ext(extensionsString, next - extensionsString);

            // scan the ignore list
            bool shouldIgnore = false;
            for (vector<string>::const_iterator iter = ignoreExt.begin();
                 iter != ignoreExt.end(); iter++)
            {
                if (*iter == ext)
                {
                    shouldIgnore = true;
                    break;
                }
            }

            if (!shouldIgnore)
                extensions.insert(extensions.end(), ext);

            if (*next == '\0')
                break;
            next++;
            extensionsString = next;
        }
    }

    // Initialize all extensions used
    for (vector<string>::const_iterator iter = extensions.begin();
         iter != extensions.end(); iter++)
    {
        InitExtension(iter->c_str());
    }

    if (extensionSupported("GL_ARB_multitexture") &&
        glx::glActiveTextureARB != NULL)
    {
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,
                      (GLint*) &maxSimultaneousTextures);
    }

    if (extensionSupported("GL_ARB_vertex_program") &&
        glx::glGenProgramsARB)
    {
        DPRINTF(1, "Renderer: ARB vertex programs supported.\n");
        if (vpARB == NULL)
            vpARB = vp::initARB();
        vertexProc = vpARB;
    }
    else if (extensionSupported("GL_NV_vertex_program") &&
             glx::glGenProgramsNV)
    {
        DPRINTF(1, "Renderer: nVidia vertex programs supported.\n");
        if (vpNV == NULL)
            vpNV = vp::initNV();
        vertexProc = vpNV;
    }
}


bool GLContext::setRenderPath(GLRenderPath path)
{
    if (!renderPathSupported(path))
        return false;

    switch (path)
    {
    case GLPath_Basic:
    case GLPath_Multitexture:
    case GLPath_NvCombiner:
        vertexPath = VPath_Basic;
        break;
    case GLPath_NvCombiner_NvVP:
        vertexPath = VPath_NV;
        break;
    case GLPath_DOT3_ARBVP:
    case GLPath_NvCombiner_ARBVP:
    case GLPath_ARBFP_ARBVP:
    case GLPath_NV30:
        vertexPath = VPath_ARB;
        break;
    default:
        return false;
    }

    renderPath = path;

    return true;
}


bool GLContext::renderPathSupported(GLRenderPath path) const
{
    switch (path)
    {
    case GLPath_Basic:
        return true;

    case GLPath_Multitexture:
        return (maxSimultaneousTextures > 1 &&
                extensionSupported("GL_EXT_texture_env_combine"));

    case GLPath_NvCombiner:
        return extensionSupported("GL_NV_register_combiners");

    case GLPath_DOT3_ARBVP:
        return (extensionSupported("GL_ARB_texture_env_dot3") &&
                extensionSupported("GL_ARB_vertex_program") &&
                vertexProc != NULL);

    case GLPath_NvCombiner_NvVP:
        // If ARB_vertex_program is supported, don't report support for
        // this render path.
        return (extensionSupported("GL_NV_register_combiners") &&
                extensionSupported("GL_NV_vertex_program") &&
                !extensionSupported("GL_ARB_vertex_program") &&
                vertexProc != NULL);

    case GLPath_NvCombiner_ARBVP:
        return (extensionSupported("GL_NV_register_combiners") &&
                extensionSupported("GL_ARB_vertex_program") &&
                vertexProc != NULL);

    case GLPath_ARBFP_ARBVP:
        return (extensionSupported("GL_ARB_vertex_program") &&
                extensionSupported("GL_ARB_fragment_program") &&
                vertexProc != NULL);

    case GLPath_NV30:
        return false;
        /*
        return (extensionSupported("GL_ARB_vertex_program") &&
                extensionSupported("GL_NV_fragment_program"));
        */

    default:
        return false;
    }
}


GLContext::GLRenderPath GLContext::nextRenderPath()
{
    GLContext::GLRenderPath newPath = renderPath;

    do {
        newPath = (GLRenderPath) ((int) newPath + 1);;
        if (newPath > GLPath_NV30)
            newPath = GLPath_Basic;
    } while (newPath != renderPath && !renderPathSupported(newPath));

    renderPath = newPath;

    return renderPath;
}


bool GLContext::extensionSupported(const string& ext) const
{
    return (find(extensions.begin(), extensions.end(), ext) != extensions.end());
}


bool GLContext::bumpMappingSupported() const
{
    return renderPath > GLPath_Multitexture;
}


GLContext::VertexPath GLContext::getVertexPath() const
{
    return vertexPath;
}


VertexProcessor* GLContext::getVertexProcessor() const
{
    return vertexProc;
}
