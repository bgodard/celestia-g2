// cmodfix.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Perform various adjustments to a cmod file

#include <celengine/modelfile.h>
#include <celengine/tokenizer.h>
#include <celengine/texmanager.h>
#include <cel3ds/3dsread.h>
#include <celmath/mathlib.h>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

using namespace std;

string inputFilename;
string outputFilename;
bool outputBinary = false;
bool uniquify = false;
bool genNormals = false;
bool weldVertices = false;
float smoothAngle = 60.0f;


void usage()
{
    cerr << "Usage: cmodfix [options] [input cmod file [output cmod file]]\n";
    cerr << "   --binary (or -b)      : output a binary .cmod file\n";
    cerr << "   --ascii (or -a)       : output an ASCII .cmod file\n";
    cerr << "   --uniquify (or -u)    : eliminate duplicate vertices\n";
    cerr << "   --normals (or -n)     : generate normals\n";
    cerr << "   --smooth (or -s) <angle> : smoothing angle for normal generation\n";
    cerr << "   --weld (or -w)        : merge identical vertices before normal generation\n";
}


struct Vertex
{
    Vertex() :
        index(0), attributes(NULL) {};

    Vertex(uint32 _index, const void* _attributes) :
        index(_index), attributes(_attributes) {};

    uint32 index;
    const void* attributes;
};


struct Face
{
    Vec3f normal;
    uint32 i[3];    // vertex attribute indices
    uint32 vi[3];   // vertex point indices -- same as above unless welding
};


typedef public std::binary_function<const Vertex&, const Vertex&, bool> VertexComparator;


class FullComparator : public VertexComparator
{
public:
    FullComparator(int _vertexSize) :
        vertexSize(_vertexSize)
    {
    }

    bool operator()(const Vertex& a, const Vertex& b) const
    {
        const char* s0 = reinterpret_cast<const char*>(a.attributes);
        const char* s1 = reinterpret_cast<const char*>(b.attributes);
        for (int i = 0; i < vertexSize; i++)
        {
            if (s0[i] < s1[i])
                return true;
            else if (s0[i] > s1[i])
                return false;
        }

        return false;
    }

private:
    int vertexSize;
};


class PointComparator : public VertexComparator
{
public:
    PointComparator()
    {
    }

    bool operator()(const Vertex& a, const Vertex& b) const
    {
        const Point3f* p0 = reinterpret_cast<const Point3f*>(a.attributes);
        const Point3f* p1 = reinterpret_cast<const Point3f*>(b.attributes);

        if (p0->x < p1->x)
        {
            return true;
        }
        else if (p0->x > p1->x)
        {
            return false;
        }
        else
        {
            if (p0->y < p1->y)
                return true;
            else if (p0->y > p1->y)
                return false;
            else
                return p0->z < p1->z;
        }
    }

private:
    int ignore;
};


class PointTexCoordComparator : public VertexComparator
{
public:
    PointTexCoordComparator(uint32 _posOffset,
                            uint32 _texCoordOffset,
                            bool _wrap) :
        posOffset(_posOffset),
        texCoordOffset(_texCoordOffset),
        wrap(_wrap)
    {
    }

    bool operator()(const Vertex& a, const Vertex& b) const
    {
        const char* adata = reinterpret_cast<const char*>(a.attributes);
        const char* bdata = reinterpret_cast<const char*>(b.attributes);
        const Point3f* p0 = reinterpret_cast<const Point3f*>(adata + posOffset);
        const Point3f* p1 = reinterpret_cast<const Point3f*>(bdata + posOffset);
        const Point2f* tc0 = reinterpret_cast<const Point2f*>(adata + posOffset);
        const Point2f* tc1 = reinterpret_cast<const Point2f*>(bdata + posOffset);
        if (p0->x < p1->x)
        {
            return true;
        }
        else if (p0->x > p1->x)
        {
            return false;
        }
        else
        {
            if (p0->y < p1->y)
            {
                return true;
            }
            else if (p0->y > p1->y)
            {
                return false;
            }
            else
            {
                if (p0->z < p1->z)
                {
                    return true;
                }
                else if (p0->z > p1->z)
                {
                    return false;
                }
                else
                {
                    if (tc0->x < tc1->x)
                        return true;
                    else if (tc0->x > tc1->x)
                        return false;
                    else
                        return tc0->y < tc1->y;
                }
            }
        }
    }

private:
    uint32 posOffset;
    uint32 texCoordOffset;
    bool wrap;
};


bool equal(const Vertex& a, const Vertex& b, uint32 vertexSize)
{
    const char* s0 = reinterpret_cast<const char*>(a.attributes);
    const char* s1 = reinterpret_cast<const char*>(b.attributes);

    for (uint32 i = 0; i < vertexSize; i++)
    {
        if (s0[i] != s1[i])
            return false;
    }

    return true;
}


bool equalPoint(const Vertex& a, const Vertex& b)
{
    const Point3f* p0 = reinterpret_cast<const Point3f*>(a.attributes);
    const Point3f* p1 = reinterpret_cast<const Point3f*>(b.attributes);

    return *p0 == *p1;
}


bool uniquifyVertices(Mesh& mesh)
{
    uint32 nVertices = mesh.getVertexCount();
    const Mesh::VertexDescription& desc = mesh.getVertexDescription();

    if (nVertices == 0)
        return false;

    const char* vertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    if (vertexData == NULL)
        return false;

    // Initialize the array of vertices
    vector<Vertex> vertices(nVertices);
    uint32 i;
    for (i = 0; i < nVertices; i++)
    {
        vertices[i] = Vertex(i, vertexData + i * desc.stride);
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    sort(vertices.begin(), vertices.end(), FullComparator(desc.stride));

    // Count the number of unique vertices
    uint32 uniqueVertexCount = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
            uniqueVertexCount++;
    }

    // No work left to do if we couldn't eliminate any vertices
    if (uniqueVertexCount == nVertices)
        return true;

    // Build the vertex map and the uniquified vertex data
    vector<uint32> vertexMap(nVertices);
    char* newVertexData = new char[uniqueVertexCount * desc.stride];
    const char* oldVertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    uint32 j = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
        {
            if (i != 0)
                j++;
            assert(j < uniqueVertexCount);
            memcpy(newVertexData + j * desc.stride,
                   oldVertexData + vertices[i].index * desc.stride,
                   desc.stride);
        }
        vertexMap[vertices[i].index] = j;
    }

    // Replace the vertex data with the compacted data
    delete mesh.getVertexData();
    mesh.setVertices(uniqueVertexCount, newVertexData);

    mesh.remapIndices(vertexMap);

    return true;
}


Point3f
getVertex(const void* vertexData,
          int positionOffset,
          uint32 stride,
          uint32 index)
{
    const float* fdata = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertexData) + stride * index + positionOffset);
    
    return Point3f(fdata[0], fdata[1], fdata[2]);
}


Point2f
getTexCoord(const void* vertexData,
            int texCoordOffset,
            uint32 stride,
            uint32 index)
{
    const float* fdata = reinterpret_cast<const float*>(reinterpret_cast<const char*>(vertexData) + stride * index + texCoordOffset);
    
    return Point2f(fdata[0], fdata[1]);
}


Vec3f
averageNormals(const vector<Face>& faces,
               uint32 thisFace,
               uint32* vertexFaces,
               uint32 vertexFaceCount,
               float cosSmoothingAngle)
{
    const Face& face = faces[thisFace];

    Vec3f v = Vec3f(0, 0, 0);
    for (uint32 i = 0; i < vertexFaceCount; i++)
    {
        uint32 f = vertexFaces[i];
        float cosAngle = face.normal * faces[f].normal;
        if (f == thisFace || cosAngle > cosSmoothingAngle)
            v += faces[f].normal;
    }

    if (v * v == 0.0f)
        v = Vec3f(1.0f, 0.0f, 0.0f);
    else
        v.normalize();

    return v;
}


void
copyVertex(void* newVertexData,
           const Mesh::VertexDescription& newDesc,
           const void* oldVertexData,
           const Mesh::VertexDescription& oldDesc,
           uint32 oldIndex,
           const uint32 fromOffsets[])
{
    const char* oldVertex = reinterpret_cast<const char*>(oldVertexData) +
        oldDesc.stride * oldIndex;
    char* newVertex = reinterpret_cast<char*>(newVertexData);

    for (uint32 i = 0; i < newDesc.nAttributes; i++)
    {
        if (fromOffsets[i] != ~0)
        {
            memcpy(newVertex + newDesc.attributes[i].offset,
                   oldVertex + fromOffsets[i],
                   Mesh::getVertexAttributeSize(newDesc.attributes[i].format));
        }
    }
}


void
augmentVertexDescription(Mesh::VertexDescription& desc,
                         Mesh::VertexAttributeSemantic semantic,
                         Mesh::VertexAttributeFormat format)
{
    Mesh::VertexAttribute* attributes = new Mesh::VertexAttribute[desc.nAttributes + 1];
    uint32 stride = 0;
    uint32 nAttributes = 0;
    bool foundMatch = false;
    
    for (uint32 i = 0; i < desc.nAttributes; i++)
    {
        if (semantic == desc.attributes[i].semantic &&
            format != desc.attributes[i].format)
        {
            // The semantic matches, but the format does not; skip this
            // item.
        }
        else
        {
            if (semantic == desc.attributes[i].semantic)
                foundMatch = true;

            attributes[nAttributes] = desc.attributes[i];
            attributes[nAttributes].offset = stride;
            stride += Mesh::getVertexAttributeSize(desc.attributes[i].format);
            nAttributes++;
        }
    }

    if (!foundMatch)
    {
        attributes[nAttributes++] = Mesh::VertexAttribute(Mesh::Normal,
                                                          Mesh::Float3,
                                                          stride);
        stride += Mesh::getVertexAttributeSize(Mesh::Float3);
    }

    delete[] desc.attributes;
    desc.attributes = attributes;
    desc.nAttributes = nAttributes;
    desc.stride = stride;
}


template <typename T> void
mergeVertices(vector<Face>& faces,
              const void* vertexData,
              const Mesh::VertexDescription& desc,
              T& comparator)
{
    // Don't do anything if we're given no data
    if (faces.size() == 0)
        return;

    // Must have a position
    if (desc.getAttribute(Mesh::Position) == NULL)
        return;
    assert(desc.getAttribute(Mesh::Position)->format == Mesh::Float3);

    uint32 posOffset = desc.getAttribute(Mesh::Position)->offset;
    const char* vertexPoints = reinterpret_cast<const char*>(vertexData) +
        posOffset;
    uint32 nVertices = faces.size() * 3;

    // Initialize the array of vertices
    vector<Vertex> vertices(nVertices);
    uint32 f;
    for (f = 0; f < faces.size(); f++)
    {
        for (uint32 j = 0; j < 3; j++)
        {
            uint32 index = faces[f].i[j];
            vertices[f * 3 + j] = Vertex(index,
                                         vertexPoints + desc.stride * index);
                                         
        }
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    sort(vertices.begin(), vertices.end(), comparator);

    // Build the vertex merge map
    vector<uint32> mergeMap(nVertices);
    uint32 lastUnique = 0;
    for (uint32 i = 0; i < nVertices; i++)
    {
        if (i == 0 ||
            comparator.operator()(vertices[i - 1], vertices[i]) ||
            comparator.operator()(vertices[i], vertices[i - 1]))
        {
            lastUnique = i;
        }
        mergeMap[vertices[i].index] = vertices[lastUnique].index;
    }

    // Remap the vertex indices
    for (f = 0; f < faces.size(); f++)
    {
        for (uint32 k= 0; k < 3; k++)
            faces[f].vi[k] = mergeMap[faces[f].i[k]];
    }
}


Mesh*
generateNormals(Mesh& mesh,
                float smoothAngle,
                bool weld)
{
    uint32 nVertices = mesh.getVertexCount();
    float cosSmoothAngle = (float) cos(smoothAngle);

    const Mesh::VertexDescription& desc = mesh.getVertexDescription();
    if (desc.getAttribute(Mesh::Position) == NULL)
    {
        cerr << "Bad vertex format--no position!\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Position)->format != Mesh::Float3)
    {
        cerr << "Vertex position must be a float3\n";
        return NULL;
    }
    uint32 posOffset = desc.getAttribute(Mesh::Position)->offset;
 
    uint32 nFaces = 0;
    uint32 i;
    for (i = 0; mesh.getGroup(i) != NULL; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);
        
        switch (group->prim)
        {
        case Mesh::TriList:
            if (group->nIndices < 3 || group->nIndices % 3 != 0)
            {
                cerr << "Triangle list has invalid number of indices\n";
                return NULL;
            }
            nFaces += group->nIndices / 3;
            break;

        case Mesh::TriStrip:
        case Mesh::TriFan:
            if (group->nIndices < 3)
            {
                cerr << "Error: tri strip or fan has less than three indices\n";
                return NULL;
            }
            nFaces += group->nIndices - 2;
            break;

        default:
            cerr << "Cannot generate normals for non-triangle primitives\n";
            return NULL;
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    vector<Face> faces(nFaces);

    uint32 f = 0;
    for (i = 0; mesh.getGroup(i) != NULL; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);
        
        switch (group->prim)
        {
        case Mesh::TriList:
            {
                for (uint32 j = 0; j < group->nIndices / 3; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[j * 3];
                    faces[f].i[1] = group->indices[j * 3 + 1];
                    faces[f].i[2] = group->indices[j * 3 + 2];
                    f++;
                }
            }
            break;

        case Mesh::TriStrip:
            {
                for (uint32 j = 2; j < group->nIndices; j++)
                {
                    assert(f < nFaces);
                    if (j % 2 == 0)
                    {
                        faces[f].i[0] = group->indices[j - 2];
                        faces[f].i[1] = group->indices[j - 1];
                        faces[f].i[2] = group->indices[j];
                    }
                    else
                    {
                        faces[f].i[0] = group->indices[j - 1];
                        faces[f].i[1] = group->indices[j - 2];
                        faces[f].i[2] = group->indices[j];
                    }
                    f++;
                }
            }
            break;

        case Mesh::TriFan:
            {
                for (uint32 j = 2; j < group->nIndices; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[0];
                    faces[f].i[1] = group->indices[j - 1];
                    faces[f].i[2] = group->indices[j];
                    f++;
                }
            }
            break;

        default:
            assert(0);
            break;
        }
    }
    assert(f == nFaces);

    const void* vertexData = mesh.getVertexData();

    // Compute normals for the faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Point3f p0 = getVertex(vertexData, posOffset, desc.stride, face.i[0]);
        Point3f p1 = getVertex(vertexData, posOffset, desc.stride, face.i[1]);
        Point3f p2 = getVertex(vertexData, posOffset, desc.stride, face.i[2]);
        face.normal = cross(p1 - p0, p2 - p1);
        if (face.normal * face.normal > 0.0f)
            face.normal.normalize();
    }

    // For each vertex, create a list of faces that contain it
    uint32* faceCounts = new uint32[nVertices];
    uint32** vertexFaces = new uint32*[nVertices];

    // Initialize the lists
    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // If we're welding vertices before generating normals, find identical
    // points and merge them.  Otherwise, the point indices will be the same
    // as the attribute indices.
    if (weld)
    {
        mergeVertices(faces, vertexData, desc, PointComparator());
    }
    else
    {
        for (f = 0; f < nFaces; f++)
        {
            faces[f].vi[0] = faces[f].i[0];
            faces[f].vi[1] = faces[f].i[1];
            faces[f].vi[2] = faces[f].i[2];
        }
    }

    // Count the number of faces in which each vertex appears
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        faceCounts[face.vi[0]]++;
        faceCounts[face.vi[1]]++;
        faceCounts[face.vi[2]]++;
    }

    // Allocate space for the per-vertex face lists
    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            vertexFaces[i] = new uint32[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }
    }

    // Fill in the vertex/face lists
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        vertexFaces[face.vi[0]][faceCounts[face.vi[0]]--] = f;
        vertexFaces[face.vi[1]][faceCounts[face.vi[1]]--] = f;
        vertexFaces[face.vi[2]][faceCounts[face.vi[2]]--] = f;
    }

    // Compute the vertex normals by averaging
    vector<Vec3f> vertexNormals(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (uint32 j = 0; j < 3; j++)
        {
            vertexNormals[f * 3 + j] =
                averageNormals(faces, f,
                               &vertexFaces[face.vi[j]][1],
                               vertexFaces[face.vi[j]][0],
                               cosSmoothAngle);
        }
    }

    // Finally, create a new mesh with normals included

    // Create the new vertex description
    Mesh::VertexDescription newDesc(desc);
    augmentVertexDescription(newDesc, Mesh::Normal, Mesh::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    uint32 normalOffset = 0;
    uint32 fromOffsets[16];
    for (i = 0; i < newDesc.nAttributes; i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == Mesh::Normal)
        {
            normalOffset = newDesc.attributes[i].offset;
        }
        else
        {
            for (uint32 j = 0; j < desc.nAttributes; j++)
            {
                if (desc.attributes[j].semantic == newDesc.attributes[i].semantic)
                {
                    assert(desc.attributes[j].format == newDesc.attributes[i].format);
                    fromOffsets[i] = desc.attributes[j].offset;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated normals to the
    // new vertex data buffer.
    void* newVertexData = new char[newDesc.stride * nFaces * 3];
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (uint32 j = 0; j < 3; j++)
        {
            char* newVertex = reinterpret_cast<char*>(newVertexData) +
                (f * 3 + j) * newDesc.stride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            memcpy(newVertex + normalOffset, &vertexNormals[f * 3 + j],
                   Mesh::getVertexAttributeSize(Mesh::Float3));
        }
    }

    // Create the Celestia mesh
    Mesh* newMesh = new Mesh();
    newMesh->setVertexDescription(newDesc);
    newMesh->setVertices(nFaces * 3, newVertexData);

    // Create a trivial index list
    uint32* indices = new uint32[nFaces * 3];
    for (i = 0; i < nFaces * 3; i++)
        indices[i] = i;

    // TODO: This assumes that the mesh uses only one material.  Normal
    // generation should really be done one primitive group at a time.
    uint32 materialIndex = mesh.getGroup(0)->materialIndex;
    newMesh->addGroup(Mesh::TriList, materialIndex, nFaces * 3, indices);

    // Clean up
    delete[] faceCounts;
    for (i = 0; i < nVertices; i++)
    {
        if (vertexFaces[i] != NULL)
            delete[] vertexFaces[i];
    }
    delete[] vertexFaces;

    return newMesh;
}


Mesh*
generateTangents(Mesh& mesh,
                 bool weld)
{
    uint32 nVertices = mesh.getVertexCount();

    // In order to generate tangents, we require positions, normals, and
    // 2D texture coordinates in the vertex description.
    const Mesh::VertexDescription& desc = mesh.getVertexDescription();
    if (desc.getAttribute(Mesh::Position) == NULL)
    {
        cerr << "Bad vertex format--no position!\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Position)->format != Mesh::Float3)
    {
        cerr << "Vertex position must be a float3\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Normal) == NULL)
    {
        cerr << "Normals must be present in mesh to generate tangents\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Normal)->format != Mesh::Float3)
    {
        cerr << "Vertex normal must be a float3\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Texture0) == NULL)
    {
        cerr << "Texture coordinates must be present in mesh to generate tangents\n";
        return NULL;
    }

    if (desc.getAttribute(Mesh::Texture0)->format != Mesh::Float2)
    {
        cerr << "Texture coordinate must be a float2\n";
        return NULL;
    }

    // Count the number of faces in the mesh.
    // (All geometry should already converted to triangle lists)
    uint32 i;
    uint32 nFaces = 0;
    for (i = 0; mesh.getGroup(i) != NULL; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);
        if (group->prim == Mesh::TriList)
        {
            assert(group->nIndices % 3 == 0);
            nFaces += group->nIndices / 3;
        }
        else
        {
            cerr << "Mesh should contain just triangle lists\n";
            return NULL;
        }
    }
    
    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    vector<Face> faces(nFaces);

    uint32 f = 0;
    for (i = 0; mesh.getGroup(i) != NULL; i++)
    {
        const Mesh::PrimitiveGroup* group = mesh.getGroup(i);
        
        switch (group->prim)
        {
        case Mesh::TriList:
            {
                for (uint32 j = 0; j < group->nIndices / 3; j++)
                {
                    assert(f < nFaces);
                    faces[f].i[0] = group->indices[j * 3];
                    faces[f].i[1] = group->indices[j * 3 + 1];
                    faces[f].i[2] = group->indices[j * 3 + 2];
                    f++;
                }
            }
            break;
        }
    }

    uint32 posOffset = desc.getAttribute(Mesh::Position)->offset;
    uint32 normOffset = desc.getAttribute(Mesh::Normal)->offset;
    uint32 texCoordOffset = desc.getAttribute(Mesh::Texture0)->offset;

    const void* vertexData = mesh.getVertexData();
    
    // Compute tangents for faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Point3f p0 = getVertex(vertexData, posOffset, desc.stride, face.i[0]);
        Point3f p1 = getVertex(vertexData, posOffset, desc.stride, face.i[1]);
        Point3f p2 = getVertex(vertexData, posOffset, desc.stride, face.i[2]);
        Point2f tc0 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[0]);
        Point2f tc1 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[1]);
        Point2f tc2 = getTexCoord(vertexData, texCoordOffset, desc.stride, face.i[2]);
        float s1 = tc1.x - tc0.x;
        float s2 = tc2.x - tc0.x;
        float t1 = tc1.y - tc0.y;
        float t2 = tc2.y - tc0.y;
        float a = s1 * t2 - s2 * t1;
        if (a != 0.0f)
            face.normal = (t2 * (p1 - p0) - t1 * (p2 - p0)) * (1.0f / a);
        else
            face.normal = Vec3f(0.0f, 0.0f, 0.0f);
    }

    // For each vertex, create a list of faces that contain it
    uint32* faceCounts = new uint32[nVertices];
    uint32** vertexFaces = new uint32*[nVertices];

    // Initialize the lists
    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // If we're welding vertices before generating normals, find identical
    // points and merge them.  Otherwise, the point indices will be the same
    // as the attribute indices.
    if (weld)
    {
        mergeVertices(faces, vertexData, desc, PointTexCoordComparator(0, 0, true));
    }
    else
    {
        for (f = 0; f < nFaces; f++)
        {
            faces[f].vi[0] = faces[f].i[0];
            faces[f].vi[1] = faces[f].i[1];
            faces[f].vi[2] = faces[f].i[2];
        }
    }

    // Count the number of faces in which each vertex appears
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        faceCounts[face.vi[0]]++;
        faceCounts[face.vi[1]]++;
        faceCounts[face.vi[2]]++;
    }

    // Allocate space for the per-vertex face lists
    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            vertexFaces[i] = new uint32[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }
    }

    // Fill in the vertex/face lists
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        vertexFaces[face.vi[0]][faceCounts[face.vi[0]]--] = f;
        vertexFaces[face.vi[1]][faceCounts[face.vi[1]]--] = f;
        vertexFaces[face.vi[2]][faceCounts[face.vi[2]]--] = f;
    }

    // Compute the vertex tangents by averaging
    vector<Vec3f> vertexNormals(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (uint32 j = 0; j < 3; j++)
        {
            vertexNormals[f * 3 + j] =
                averageNormals(faces, f,
                               &vertexFaces[face.vi[j]][1],
                               vertexFaces[face.vi[j]][0],
                               0.0f);
        }
    }

    // Create the new vertex description
    Mesh::VertexDescription newDesc(desc);
    augmentVertexDescription(newDesc, Mesh::Tangent, Mesh::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    uint32 tangentOffset = 0;
    uint32 fromOffsets[16];
    for (i = 0; i < newDesc.nAttributes; i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == Mesh::Tangent)
        {
            tangentOffset = newDesc.attributes[i].offset;
        }
        else
        {
            for (uint32 j = 0; j < desc.nAttributes; j++)
            {
                if (desc.attributes[j].semantic == newDesc.attributes[i].semantic)
                {
                    assert(desc.attributes[j].format == newDesc.attributes[i].format);
                    fromOffsets[i] = desc.attributes[j].offset;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated tangents to the
    // new vertex data buffer.
    void* newVertexData = new char[newDesc.stride * nFaces * 3];
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (uint32 j = 0; j < 3; j++)
        {
            char* newVertex = reinterpret_cast<char*>(newVertexData) +
                (f * 3 + j) * newDesc.stride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            memcpy(newVertex + tangentOffset, &vertexNormals[f * 3 + j],
                   Mesh::getVertexAttributeSize(Mesh::Float3));
        }
    }

    return NULL;
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--binary"))
            {
                outputBinary = true;
            }
            else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--ascii"))
            {
                outputBinary = false;
            }
            else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--uniquify"))
            {
                uniquify = true;
            }
            else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--normals"))
            {
                genNormals = true;
            }
            else if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "--weld"))
            {
                weldVertices = true;
            }
            else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--smooth"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %f", &smoothAngle) != 1)
                        return false;
                    i++;
                }
            }
            else
            {
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                inputFilename = string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than two filenames on the command line is an error
                return false;
            }
            i++;
        }
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv))
    {
        usage();
        return 1;
    }

    Model* model = NULL;
    if (!inputFilename.empty())
    {
        ifstream in(inputFilename.c_str(), ios::in | ios::binary);
        if (!in.good())
        {
            cerr << "Error opening " << inputFilename << "\n";
            return 1;
        }
        model = LoadModel(in);
    }
    else
    {
        model = LoadModel(cin);
    }
    
    if (model == NULL)
        return 1;
    
    if (genNormals)
    {
        Model* normGenModel = new Model();
        uint32 i;

        // Copy materials
        for (i = 0; model->getMaterial(i) != NULL; i++)
        {
            normGenModel->addMaterial(model->getMaterial(i));
        }

        // Generate normals for each model in the mesh
        for (i = 0; model->getMesh(i) != NULL; i++)
        {
            Mesh* mesh = model->getMesh(i);
            Mesh* normGenMesh = generateNormals(*mesh,
                                                degToRad(smoothAngle),
                                                weldVertices);
            if (normGenMesh == NULL)
            {
                cerr << "Error generating normals!\n";
                return 1;
            }

            normGenModel->addMesh(normGenMesh);
        }

        // delete model;
        model = normGenModel;
    }

    if (uniquify)
    {
        for (uint32 i = 0; model->getMesh(i) != NULL; i++)
        {
            Mesh* mesh = model->getMesh(i);
            uniquifyVertices(*mesh);
        }
    }

    if (outputFilename.empty())
    {
        if (outputBinary)
            SaveModelBinary(model, cout);
        else
            SaveModelAscii(model, cout);
    }
    else
    {
        ofstream out(outputFilename.c_str(),
                     ios::out | (outputBinary ? ios::binary : 0));
        if (!out.good())
        {
            cerr << "Error opening output file " << outputFilename << "\n";
            return 1;
        }

        if (outputBinary)
            SaveModelBinary(model, out);
        else
            SaveModelAscii(model, out);
    }

    return 0;
}
