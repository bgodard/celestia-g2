// lodspheremesh.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <celmath/mathlib.h>
#include <celmath/vecmath.h>
#include "gl.h"
#include "glext.h"
#include "vecgl.h"
#include "lodspheremesh.h"

using namespace std;


static bool trigArraysInitialized = false;
static int maxDivisions = 2048;
static int thetaDivisions = maxDivisions;
static int phiDivisions = maxDivisions / 2;
static int minStep = 64;
static float* sinPhi = NULL;
static float* cosPhi = NULL;
static float* sinTheta = NULL;
static float* cosTheta = NULL;


static void InitTrigArrays()
{
    sinTheta = new float[thetaDivisions + 1];
    cosTheta = new float[thetaDivisions + 1];
    sinPhi = new float[phiDivisions + 1];
    cosPhi = new float[phiDivisions + 1];

    int i;
    for (i = 0; i <= thetaDivisions; i++)
    {
        double theta = (double) i / (double) thetaDivisions * 2.0 * PI;
        sinTheta[i] = (float) sin(theta);
        cosTheta[i] = (float) cos(theta);
    }

    for (i = 0; i <= phiDivisions; i++)
    {
        double phi = ((double) i / (double) phiDivisions - 0.5) * PI;
        sinPhi[i] = (float) sin(phi);
        cosPhi[i] = (float) cos(phi);
    }

    trigArraysInitialized = true;
}


LODSphereMesh::LODSphereMesh() :
    vertices(NULL),
    normals(NULL),
    tangents(NULL)
{
    if (!trigArraysInitialized)
        InitTrigArrays();

    int maxThetaSteps = thetaDivisions / minStep;
    int maxPhiSteps = phiDivisions / minStep;
    int maxVertices = (maxPhiSteps + 1) * (maxThetaSteps + 1);
    
    vertices = new float[maxVertices * 3];
    normals = new float[maxVertices * 3];
    for (int i = 0; i < MAX_SPHERE_MESH_TEXTURES; i++)
        texCoords[i] = new float[maxVertices * 2];
    tangents = new float[maxVertices * 3];

    indices = new unsigned short[maxPhiSteps * 2 * (maxThetaSteps + 1)];
}

LODSphereMesh::~LODSphereMesh()
{
    if (vertices != NULL)
        delete[] vertices;
    if (normals != NULL)
        delete[] normals;
    if (tangents != NULL)
        delete[] tangents;
    for (int i = 0; i < MAX_SPHERE_MESH_TEXTURES; i++)
    {
        if (texCoords != NULL)
            delete[] texCoords[i];
    }
}


static Point3f spherePoint(int theta, int phi)
{
    return Point3f(cosPhi[phi] * cosTheta[theta],
                   sinPhi[phi],
                   cosPhi[phi] * sinTheta[theta]);
}


void LODSphereMesh::render(const GLContext& context,
                           const Frustum& frustum,
                           float lodBias,
                           Texture** tex,
                           int nTextures)
{
    render(context,
           Mesh::Normals | Mesh::TexCoords0, frustum, lodBias, tex,
           nTextures);
}


void LODSphereMesh::render(const GLContext& context,
                           unsigned int attributes,
                           const Frustum& frustum,
                           float lodBias,
                           Texture* tex0,
                           Texture* tex1,
                           Texture* tex2,
                           Texture* tex3)
{
    Texture* textures[MAX_SPHERE_MESH_TEXTURES];
    int nTextures = 0;

    if (tex0 != NULL)
        textures[nTextures++] = tex0;
    if (tex1 != NULL)
        textures[nTextures++] = tex1;
    if (tex2 != NULL)
        textures[nTextures++] = tex2;
    if (tex3 != NULL)
        textures[nTextures++] = tex3;
    render(context, attributes, frustum, lodBias, textures, nTextures);
}


void LODSphereMesh::render(const GLContext& context,
                           unsigned int attributes,
                           const Frustum& frustum,
                           float lodBias,
                           Texture** tex,
                           int nTextures)
{
    Point3f fp[8];

    int lod = 64;
    if (lodBias < 0.0f)
    {
        if (lodBias < -30)
            lodBias = -30;
        lod = lod / (1 << (int) (-lodBias));
        if (lod < 2)
            lod = 2;
    }
    else if (lodBias > 0.0f)
    {
        if (lodBias > 30)
            lodBias = 30;
        lod = lod * (1 << (int) lodBias);
        if (lod > maxDivisions)
            lod = maxDivisions;
    }

    int step = maxDivisions / lod;
    int thetaExtent = maxDivisions;
    int phiExtent = thetaExtent / 2;

    int split = 1;
    if (step < minStep)
    {
        split = minStep / step;
        thetaExtent /= split;
        phiExtent /= split;
    }

    if (tex == NULL)
        nTextures = 0;

    // If one of the textures is split into subtextures, we may have to
    // use extra patches, since there can be at most one subtexture per patch.
    int minSplit = 1;
    int i;
    for (i = 0; i < nTextures; i++)
    {
        if (tex[i]->getUSubtextures() > minSplit)
            minSplit = tex[i]->getUSubtextures();
        if (tex[i]->getVSubtextures() > minSplit)
            minSplit = tex[i]->getVSubtextures();
    }
    
    if (split < minSplit)
    {
        thetaExtent /= (minSplit / split);
        phiExtent /= (minSplit / split);
        split = minSplit;
    }

    // Set the current textures
    nTexturesUsed = nTextures;
    for (i = 0; i < nTextures; i++)
    {
        textures[i] = tex[i];
        subtextures[i] = 0;
        if (nTextures > 1)
            glx::glActiveTextureARB(GL_TEXTURE0_ARB + i);
        glEnable(GL_TEXTURE_2D);
    }

    // Set up the mesh vertices 
    int nRings = phiExtent / step;
    int nSlices = thetaExtent / step;

    int n2 = 0;
    for (i = 0; i < nRings; i++)
    {
        for (int j = 0; j <= nSlices; j++)
        {
            indices[n2 + 0] = i * (nSlices + 1) + j;
            indices[n2 + 1] = (i + 1) * (nSlices + 1) + j;
            n2 += 2;
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);

    if ((attributes & Mesh::Normals) != 0)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, vertices);
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

#if 0
    if (texCoords != NULL && ((attributes & Mesh::TexCoords0) != 0))
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
#endif

    for (i = 0; i < nTextures; i++)
    {
        if (nTextures > 1)
            glx::glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords[i]);
    }

    glDisableClientState(GL_COLOR_ARRAY);

    // Use nVidia's vertex program extension . . .  right now, we
    // just assume that we only send down tangents if we're using this
    // extension.  Need to come up with a better solution . . .
    if (tangents != NULL && ((attributes & Mesh::Tangents) != 0))
    {
#if 0
        glEnableClientState(GL_VERTEX_ATTRIB_ARRAY6_NV);
        glx::glVertexAttribPointerNV(6, 3, GL_FLOAT, 0, tangents);
        glx::glEnableVertexAttribArrayARB(6);
        glx::glVertexAttribPointerARB(6, 3, GL_FLOAT, GL_FALSE, 0, tangents);
#endif
        VertexProcessor* vproc = context.getVertexProcessor();
        if (vproc != NULL)
        {
            vproc->enableAttribArray(6);
            vproc->attribArray(6, 3, GL_FLOAT, 0, tangents);
        }
    }

    if (split == 1)
    {
        renderSection(0, 0, thetaExtent, step, attributes);
    }
    else
    {
        // Render the sphere section by section.
        int reject = 0;

        // Compute the vertices of the view frustum.  These will be used for
        // culling patches.
        fp[0] = Planef::intersection(frustum.getPlane(Frustum::Near),
                                     frustum.getPlane(Frustum::Top),
                                     frustum.getPlane(Frustum::Left));
        fp[1] = Planef::intersection(frustum.getPlane(Frustum::Near),
                                     frustum.getPlane(Frustum::Top),
                                     frustum.getPlane(Frustum::Right));
        fp[2] = Planef::intersection(frustum.getPlane(Frustum::Near),
                                     frustum.getPlane(Frustum::Bottom),
                                     frustum.getPlane(Frustum::Left));
        fp[3] = Planef::intersection(frustum.getPlane(Frustum::Near),
                                     frustum.getPlane(Frustum::Bottom),
                                     frustum.getPlane(Frustum::Right));
        fp[4] = Planef::intersection(frustum.getPlane(Frustum::Far),
                                     frustum.getPlane(Frustum::Top),
                                     frustum.getPlane(Frustum::Left));
        fp[5] = Planef::intersection(frustum.getPlane(Frustum::Far),
                                     frustum.getPlane(Frustum::Top),
                                     frustum.getPlane(Frustum::Right));
        fp[6] = Planef::intersection(frustum.getPlane(Frustum::Far),
                                     frustum.getPlane(Frustum::Bottom),
                                     frustum.getPlane(Frustum::Left));
        fp[7] = Planef::intersection(frustum.getPlane(Frustum::Far),
                                     frustum.getPlane(Frustum::Bottom),
                                     frustum.getPlane(Frustum::Right));
#if 0
        for (int foo = 0; foo < 8; foo++)
            cout << "x: " << fp[foo].x << "  y: " << fp[foo].y << "  z: " << fp[foo].z << '\n';
#endif

        int nPatches = 0;
        {
            int extent = maxDivisions / 2;

            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    nPatches += renderPatches(i * extent / 2, j * extent,
                                              extent,
                                              split / 2,
                                              step,
                                              attributes,
                                              fp);
                }
            }
        }
        // cout << "Rendered " << nPatches << " of " << square(split) << " patches\n";
    }

    if (tangents != NULL && ((attributes & Mesh::Tangents) != 0))
    {
#if 0
        glDisableClientState(GL_VERTEX_ATTRIB_ARRAY6_NV);
        glDisableVertexAttribArrayARB(6);
#endif
        VertexProcessor* vproc = context.getVertexProcessor();
        if (vproc != NULL)
            vproc->disableAttribArray(6);
    }

    for (i = 0; i < nTextures; i++)
    {
        if (nTextures > 1)
        {
            glx::glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
            glx::glActiveTextureARB(GL_TEXTURE0_ARB + i);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if (i > 0)
            glDisable(GL_TEXTURE_2D);
    }

    if (nTextures > 1)
    {
        glx::glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glx::glActiveTextureARB(GL_TEXTURE0_ARB);
    }

#if SHOW_FRUSTUM
    // Debugging code for visualizing the frustum.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(45.0, 1.3333f, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor4f(1, 0, 0, 1);
    glTranslatef(0, 0, -20);
    glBegin(GL_LINES);
    glVertex(fp[0]); glVertex(fp[1]);
    glVertex(fp[0]); glVertex(fp[2]);
    glVertex(fp[3]); glVertex(fp[1]);
    glVertex(fp[3]); glVertex(fp[2]);
    glVertex(fp[4]); glVertex(fp[5]);
    glVertex(fp[4]); glVertex(fp[6]);
    glVertex(fp[7]); glVertex(fp[5]);
    glVertex(fp[7]); glVertex(fp[6]);
    glVertex(fp[0]); glVertex(fp[4]);
    glVertex(fp[1]); glVertex(fp[5]);
    glVertex(fp[2]); glVertex(fp[6]);
    glVertex(fp[3]); glVertex(fp[7]);
    glEnd();

    // Render axes representing the unit sphere.
    glColor4f(0, 1, 0, 1);
    glBegin(GL_LINES);
    glVertex3f(-1, 0, 0); glVertex3f(1, 0, 0);
    glVertex3f(0, -1, 0); glVertex3f(0, 1, 0);
    glVertex3f(0, 0, -1); glVertex3f(1, 0, 1);
    glEnd();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
#endif
}


int LODSphereMesh::renderPatches(int phi0, int theta0, 
                                 int extent,
                                 int level,
                                 int step,
                                 unsigned int attributes,
                                 Point3f* fp)
{
    int thetaExtent = extent;
    int phiExtent = extent / 2;

    // Compute the plane separating this section of the sphere from
    // the rest of the sphere.  If the view frustum lies entirely
    // on the side of the plane that does not contain the sphere
    // patch, we cull the patch.
    Point3f p0 = spherePoint(theta0, phi0);
    Point3f p1 = spherePoint(theta0 + thetaExtent, phi0);
    Point3f p2 = spherePoint(theta0 + thetaExtent,
                             phi0 + phiExtent);
    Point3f p3 = spherePoint(theta0, phi0 + phiExtent);
    Vec3f v0 = p1 - p0;
    Vec3f v2 = p3 - p2;
    Vec3f normal;
    if (v0.lengthSquared() > v2.lengthSquared())
        normal = (p0 - p3) ^ v0;
    else
        normal = (p2 - p1) ^ v2;

    // If the normal is near zero length, something's going wrong
    assert(normal.length() > 1.0e-6);
    normal.normalize();
    Planef separatingPlane(normal, p0);

    bool outside = true;
    for (int k = 0; k < 8; k++)
    {
        if (separatingPlane.distanceTo(fp[k]) > 0.0f)
        {
            outside = false;
            break;
        }
    }

    // If this patch is outside the view frustum, so are all of its subpatches
    if (outside)
    {
        return 0;
    }
    else if (level == 1)
    {
        renderSection(phi0, theta0,
                      thetaExtent,
                      step, attributes);
        return 1;
    }
    else
    {
        int nRendered = 0;
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                nRendered += renderPatches(phi0 + phiExtent / 2 * i,
                                           theta0 + thetaExtent / 2 * j,
                                           extent / 2,
                                           level / 2,
                                           step,
                                           attributes,
                                           fp);
            }
        }
        return nRendered;
    }
}


void LODSphereMesh::renderSection(int phi0, int theta0,
                                  int extent,
                                  int step,
                                  unsigned int attributes)
{
    // assert(step >= minStep);
    // assert(phi0 + extent <= maxDivisions);
    // assert(theta0 + extent / 2 < maxDivisions);
    // assert(isPow2(extent));
    int thetaExtent = extent;
    int phiExtent = extent / 2;
    int theta1 = theta0 + thetaExtent;
    int phi1 = phi0 + phiExtent;
    int n3 = 0;
    int n2 = 0;

    float du[MAX_SPHERE_MESH_TEXTURES];
    float dv[MAX_SPHERE_MESH_TEXTURES];
    float u0[MAX_SPHERE_MESH_TEXTURES];
    float v0[MAX_SPHERE_MESH_TEXTURES];

    // Set the current texture.  This is necessary because the texture
    // may be split into subtextures.
    for (int tex = 0; tex < nTexturesUsed; tex++)
    {
        du[tex] = (float) 1.0f / thetaDivisions;;
        dv[tex] = (float) 1.0f / phiDivisions;;
        u0[tex] = 1.0f;
        v0[tex] = 1.0f;

        if (textures[tex] != NULL)
        {
            int uTexSplit = textures[tex]->getUSubtextures();
            int vTexSplit = textures[tex]->getVSubtextures();
            int patchSplit = maxDivisions / extent;
            assert(patchSplit >= uTexSplit && patchSplit >= vTexSplit);

            int u = theta0 / thetaExtent;
            int v = phi0 / phiExtent;
            int patchesPerUSubtex = patchSplit / uTexSplit;
            int patchesPerVSubtex = patchSplit / vTexSplit;

            du[tex] *= uTexSplit;
            dv[tex] *= vTexSplit;
            u0[tex] = 1.0f - ((float) (u % patchesPerUSubtex) /
                              (float) patchesPerUSubtex);
            v0[tex] = 1.0f - ((float) (v % patchesPerVSubtex) /
                              (float) patchesPerVSubtex);
            u0[tex] += theta0 * du[tex];
            v0[tex] += phi0 * dv[tex];

            u /= patchesPerUSubtex;
            v /= patchesPerVSubtex;

            unsigned int tn = textures[tex]->getName(uTexSplit - u - 1,
                                                     vTexSplit - v - 1);
            // We track the current texture to avoid unnecessary and costly
            // texture state changes.
            if (tn != subtextures[tex])
            {
                if (nTexturesUsed > 1)
                    glx::glActiveTextureARB(GL_TEXTURE0_ARB + tex);
                glBindTexture(GL_TEXTURE_2D, tn);
                subtextures[tex] = tn;
            }
        }
    }

    for (int phi = phi0; phi <= phi1; phi += step)
    {
        float cphi = cosPhi[phi];
        float sphi = sinPhi[phi];

        if ((attributes & Mesh::Tangents) != 0)
        {
            for (int theta = theta0; theta <= theta1; theta += step)
            {
                float ctheta = cosTheta[theta];
                float stheta = sinTheta[theta];

                vertices[n3]      = cphi * ctheta;
                vertices[n3 + 1]  = sphi;
                vertices[n3 + 2]  = cphi * stheta;

                // Compute the tangent--required for bump mapping
                tangents[n3]      = -stheta;
                tangents[n3 + 1]  = 0.0f;
                tangents[n3 + 2]  = ctheta;
                
                n2 += 2;
                n3 += 3;
            }
        }
        else
        {
            for (int theta = theta0; theta <= theta1; theta += step)
            {
                float ctheta = cosTheta[theta];
                float stheta = sinTheta[theta];

                vertices[n3]      = cphi * ctheta;
                vertices[n3 + 1]  = sphi;
                vertices[n3 + 2]  = cphi * stheta;
                n2 += 2;
                n3 += 3;
            }
        }

        // Texture coordinates
        for (int tex = 0; tex < nTexturesUsed; tex++)
        {
            float u = u0[tex];
            float v = v0[tex];
            float du_ = du[tex];
            float dv_ = dv[tex];

            n2 -= (theta1 - theta0) / step * 2 + 2;
            for (int theta = theta0; theta <= theta1; theta += step)
            {
                float ctheta = cosTheta[theta];
                float stheta = sinTheta[theta];
                texCoords[tex][n2]     = u - theta * du_;
                texCoords[tex][n2 + 1] = v - phi * dv_;
                n2 += 2;
            }
        }
    }

    int nRings = phiExtent / step;
    int nSlices = thetaExtent / step;
    for (int i = 0; i < nRings; i++)
    {
        glDrawElements(GL_QUAD_STRIP,
                       (nSlices + 1) * 2,
                       GL_UNSIGNED_SHORT,
                       indices + (nSlices + 1) * 2 * i);
    }
}
