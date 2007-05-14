// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <stdio.h>
#include "celestia.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include "astro.h"
#include "nebula.h"
#include "meshmanager.h"
#include "rendcontext.h"
#include "gl.h"
#include "glext.h"
#include "vecgl.h"
#include "render.h"

using namespace std;


Nebula::Nebula() :
    model(InvalidResource)
{
}


const char* Nebula::getType() const
{
    return "Nebula";
}


void Nebula::setType(const string& /*typeStr*/)
{
}


size_t Nebula::getDescription(char* buf, size_t bufLength) const
{
    return snprintf(buf, bufLength, _("%s"), getType());
}


ResourceHandle Nebula::getModel() const
{
    return model;
}


void Nebula::setModel(ResourceHandle _model)
{
    model = _model;
}

const char* Nebula::getObjTypeName() const
{
    return "nebula";
}


bool Nebula::pick(const Ray3d& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    // The preconditional sphere-ray intersection test is enough for now:
    return DeepSkyObject::pick(ray, distanceToPicker, cosAngleToBoundCenter);
}


bool Nebula::load(AssociativeArray* params, const string& resPath)
{
    string model;
    if (params->getString("Mesh", model))
    {
        ResourceHandle modelHandle =
            GetModelManager()->getHandle(ModelInfo(model, resPath));
        setModel(modelHandle);
    }

    return DeepSkyObject::load(params, resPath);
}


void Nebula::render(const GLContext& glcontext,
                    const Vec3f&,
                    const Quatf&,
                    float,
                    float pixelSize)
{
    Model* m = NULL;
    if (model != InvalidResource)
        m = GetModelManager()->find(model);
    if (m == NULL)
        return;

    glDisable(GL_BLEND);

    glScalef(getRadius(), getRadius(), getRadius());
    glRotate(getOrientation());

    if (glcontext.getRenderPath() == GLContext::GLPath_GLSL)
    {
        GLSLUnlit_RenderContext rc(getRadius());
        rc.setPointScale(2.0 * getRadius() / pixelSize);
        m->render(rc);
        glx::glUseProgramObjectARB(0);
    }
    else
    {
        FixedFunctionRenderContext rc;
        rc.setLighting(false);
        m->render(rc);

        // Reset the material
        float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float zero = 0.0f;
        glColor4fv(black);
        glMaterialfv(GL_FRONT, GL_EMISSION, black);
        glMaterialfv(GL_FRONT, GL_SPECULAR, black);
        glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
    }

    glEnable(GL_BLEND);
}


unsigned int Nebula::getRenderMask() const
{
    return Renderer::ShowNebulae;
}


unsigned int Nebula::getLabelMask() const
{
    return Renderer::NebulaLabels;
}
