// texture.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifdef _WIN32
#define JPEG_SUPPORT
#define PNG_SUPPORT
#endif

#include <cmath>
#include <fstream>
#include <cstdio>
#include "gl.h"
#include "glext.h"
#ifdef JPEG_SUPPORT
#include "ijl.h"
#endif
#ifdef PNG_SUPPORT
#include "setjmp.h"
#include "png.h"
#endif

#include "celestia.h"
#include "vecmath.h"
#include "filetype.h"
#include "texture.h"

using namespace std;


typedef struct
{
    unsigned char b;
    unsigned char m;
    unsigned int size;
    unsigned int reserved;
    unsigned int offset;
} BMPFileHeader;

typedef struct
{
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int imageSize;
    int widthPPM;
    int heightPPM;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPImageHeader;

static bool initialized = false;
static bool compressionSupported = false;


static void initTextureLoader()
{
    compressionSupported = ExtensionSupported("GL_ARB_texture_compression");
    initialized = true;
}


CTexture::CTexture(int w, int h, int fmt, bool _cubeMap) :
    width(w),
    height(h),
    format(fmt),
    cubeMap(_cubeMap)
{
    cmap = NULL;
    cmapEntries = 0;

    // assert(!cubeMap || height == width);

    // Yuck . . .
    if (!initialized)
        initTextureLoader();

    switch (format)
    {
    case GL_RGB:
    case GL_BGR_EXT:
        components = 3;
        break;
    case GL_RGBA:
        components = 4;
        break;
    case GL_ALPHA:
        components = 1;
        break;
    case GL_LUMINANCE:
        components = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        components = 2;
        break;
    default:
        break;
    }

    int faces = cubeMap ? 6 : 1;
    pixels = new unsigned char[width * height * components * faces];

    glName = 0;
}


CTexture::~CTexture()
{
    if (pixels != NULL)
        delete[] pixels;
    if (cmap != NULL)
        delete[] cmap;
    if (glName != 0)
        glDeleteTextures(1, &glName);
}


void CTexture::bindName(uint32 flags)
{
    bool wrap = ((flags & WrapTexture) != 0);
    bool compress = ((flags & CompressTexture) != 0) && compressionSupported;

    if (pixels == NULL)
        return;

    GLuint textureType = GL_TEXTURE_2D;
    GLuint wrapMode = wrap ? GL_REPEAT : GL_CLAMP;
    if (cubeMap)
    {
        textureType = GL_TEXTURE_CUBE_MAP_EXT;
        wrapMode = GL_CLAMP_TO_EDGE;
    }

    GLuint tn;
    glGenTextures(1, &tn);
    glBindTexture(textureType, tn);

    glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    int internalFormat = components;
    // compress = true;
    if (compress)
    {
        switch (format)
        {
        case GL_RGB:
        case GL_BGR_EXT:
            internalFormat = GL_COMPRESSED_RGB_ARB;
            break;
        case GL_RGBA:
            internalFormat = GL_COMPRESSED_RGBA_ARB;
            break;
        case GL_ALPHA:
            internalFormat = GL_COMPRESSED_ALPHA_ARB;
            break;
        case GL_LUMINANCE:
            internalFormat = GL_COMPRESSED_LUMINANCE_ARB;
            break;
        case GL_LUMINANCE_ALPHA:
            internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
            break;
        case GL_INTENSITY:
            internalFormat = GL_COMPRESSED_INTENSITY_ARB;
            break;
        }
	glHint((GLenum) GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
    }

    int nFaces = 1;
    int textureTarget = GL_TEXTURE_2D;
    if (cubeMap)
    {
        nFaces = 6;
        textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT;
    }

    for (int face = 0; face < nFaces; face++)
    {
        gluBuild2DMipmaps(textureTarget + face,
                          internalFormat,
                          width, height,
                          format,
                          GL_UNSIGNED_BYTE,
                          pixels + face * width * height * components);
    }
    
    glName = tn;

    delete pixels;
}


unsigned int CTexture::getName()
{
    return glName;
}


// Convert the texture to a normal map
void CTexture::normalMap(float scale, bool wrap)
{
    // Make sure that we get the texture after it's been loaded with
    // data, but before bindName was called and texel data deleted.
    if (pixels == NULL)
    {
        DPRINTF("Texture::normalMap: no texel data!\n");
        return;
    }

    unsigned char* npixels = new unsigned char[width * height * 4];

    // Compute normals using differences between adjacent texels.  Only
    // the value of the first channel is considered with computing
    // differences--this produces the expected results with greyscale
    // textures.
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int i0 = i;
            int j0 = j;
            int i1 = i - 1;
            int j1 = j - 1;
            if (i1 < 0)
            {
                if (wrap)
                {
                    i1 = height - 1;
                }
                else
                {
                    i0++;
                    i1++;
                }   
            }
            if (j1 < 0)
            {
                if (wrap)
                {
                    j1 = width - 1;
                }
                else
                {
                    j0++;
                    j1++;
                }
            }

            int h00 = (int) pixels[(i0 * width + j0) * components];
            int h10 = (int) pixels[(i0 * width + j1) * components];
            int h01 = (int) pixels[(i1 * width + j0) * components];
            
            float dx = (float) (h00 - h10) * (1.0f / 255.0f) * scale;
            float dy = (float) (h00 - h01) * (1.0f / 255.0f) * scale;

            float mag = (float) sqrt(dx * dx + dy * dy + 1.0f);
            float rmag = 1.0f / mag;

            int n = (i * width + j) * 4;
            // npixels[n]     = (unsigned char) (128 + 127 * dy * rmag);
            // npixels[n + 1] = (unsigned char) (128 - 127 * dx * rmag);
            npixels[n]     = (unsigned char) (128 - 127 * dx * rmag);
            npixels[n + 1] = (unsigned char) (128 + 127 * dy * rmag);
            npixels[n + 2] = (unsigned char) (128 + 127 * rmag);
            npixels[n + 3] = 255;
        }
    }

    delete[] pixels;
    pixels = npixels;

    format = GL_RGBA;
    components = 4;
    isNormalMap = true;
}


CTexture* CreateProceduralTexture(int width, int height,
                                  int format,
                                  ProceduralTexEval func)
{
    CTexture* tex = new CTexture(width, height, format);
    if (tex == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = (float) x / (float) width * 2 - 1;
            float v = (float) y / (float) height * 2 - 1;
            func(u, v, 0, tex->pixels + (y * width + x) * tex->components);
        }
    }

    return tex;
}


CTexture* LoadTextureFromFile(const string& filename)
{
    ContentType type = DetermineFileType(filename);

    switch (type)
    {
    case Content_JPEG:
        return CreateJPEGTexture(filename.c_str());
    case Content_BMP:
        return CreateBMPTexture(filename.c_str());
    case Content_PNG:
        return CreatePNGTexture(filename);
    default:
        DPRINTF("Unrecognized or unsupported image file type.\n");
        return NULL;
    }
}


CTexture* CreateJPEGTexture(const char* filename,
                            int channels)
{
#ifndef JPEG_SUPPORT
    return NULL;
#else
    JPEG_CORE_PROPERTIES jpegProps;

    printf("Reading texture: %s\n", filename);

    // Must specify at least one of color or alpha
    if (channels == 0)
        return NULL;

    ZeroMemory(&jpegProps, sizeof(JPEG_CORE_PROPERTIES));
    if (ijlInit(&jpegProps) != IJL_OK)
        return NULL;

    jpegProps.JPGFile = (char*) filename;
    if (ijlRead(&jpegProps, IJL_JFILE_READPARAMS) != IJL_OK)
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    // Set up the JPG color space, guessing based on the number of
    // color channels.
    switch (jpegProps.JPGChannels)
    {
    case 1:
        jpegProps.JPGColor = IJL_G;
        break;
    case 3:
        jpegProps.JPGColor = IJL_YCBCR;
        break;
    default:
        jpegProps.JPGColor = (IJL_COLOR) IJL_OTHER;
        break;
    }

    // Set up the target color space
    int format;
    if (jpegProps.JPGColor == IJL_YCBCR)
    {
        if ((channels & CTexture::AlphaChannel) != 0)
            format = GL_RGBA;
        else
            format = GL_RGB;
        jpegProps.DIBChannels = 3;
        jpegProps.DIBColor = IJL_RGB;
    }
    else if (jpegProps.JPGColor == IJL_G)
    {
        if ((channels & CTexture::AlphaChannel) != 0)
            format = GL_LUMINANCE_ALPHA;
        else
            format = GL_LUMINANCE;
        jpegProps.DIBChannels = 1;
        jpegProps.DIBColor = IJL_G;
    }
    else
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    // Create the texture
    CTexture* tex = new CTexture(jpegProps.JPGWidth, jpegProps.JPGHeight,
                                 format);
    if (tex == NULL)
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    jpegProps.DIBBytes = tex->pixels;
    jpegProps.DIBWidth = tex->width;
    jpegProps.DIBHeight = tex->height;

    // Slurp the body of the image
    if (ijlRead(&jpegProps, IJL_JFILE_READWHOLEIMAGE) != IJL_OK)
    {
        printf("Failed to read texture\n");
        ijlFree(&jpegProps);
        delete tex;
        return NULL;
    }

    ijlFree(&jpegProps);

    // If necessary, synthesize an alpha channel from color information
    if ((channels & CTexture::AlphaChannel) != 0)
    {
        if (format == GL_LUMINANCE_ALPHA)
        {
            int nPixels = tex->width * tex->height;
            unsigned char *newPixels = new unsigned char[nPixels * 2];
            for (int i = 0; i < nPixels; i++)
            {
                newPixels[i * 2] = newPixels[i * 2 + 1] = tex->pixels[i];
            }
            delete[] tex->pixels;
            tex->pixels = newPixels;
        }
    }
    
    return tex;
#endif // JPEG_SUPPORT
}


#ifdef PNG_SUPPORT
void PNGReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* fp = (FILE*) png_get_io_ptr(png_ptr);
    fread((void*) data, 1, length, fp);
}
#endif

CTexture* CreatePNGTexture(const string& filename)
{
#ifndef PNG_SUPPORT
    return NULL;
#else
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    int glformat;
    FILE* fp = NULL;
    CTexture* tex = NULL;
    png_bytep* row_pointers = NULL;

    fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        DPRINTF("Error opening texture file %s\n", filename.c_str());
        return NULL;
    }
   
    fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp((unsigned char*) header, 0, sizeof(header)))
    {
        DPRINTF("Error: %s is not a PNG file.\n", filename.c_str());
        fclose(fp);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        return NULL;
    }
   
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        if (tex != NULL)
            delete tex;
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        DPRINTF("Error reading PNG texture file %s\n", filename.c_str());
        return NULL;
    }

    // png_init_io(png_ptr, fp);
    png_set_read_fn(png_ptr, (void*) fp, PNGReadData);
    png_set_sig_bytes(png_ptr, sizeof(header));

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height, &bit_depth,
                 &color_type, &interlace_type,
                 NULL, NULL);
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        glformat = GL_LUMINANCE;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        glformat = GL_LUMINANCE_ALPHA;
        break;
    case PNG_COLOR_TYPE_RGB:
        glformat = GL_RGB;
        break;
    case PNG_COLOR_TYPE_PALETTE:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        glformat = GL_RGBA;
        break;
    default:
        // badness
        break;
    }

    tex = new CTexture(width, height, glformat);
    if (tex == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        return NULL;
    }

    // TODO: consider using paletted textures if they're available
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    // TODO: consider passing textures with < 8 bits/component to
    // GL without expanding
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    else if (bit_depth < 8)
        png_set_packing(png_ptr);

    row_pointers = new png_bytep[height];
    for (int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) &tex->pixels[tex->components * width * i];

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return tex;
#endif
}


static int readInt(ifstream& in)
{
    unsigned char b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    return ((int) b[3] << 24) + ((int) b[2] << 16)
        + ((int) b[1] << 8) + (int) b[0];
}


static short readShort(ifstream& in)
{
    unsigned char b[2];
    in.read(reinterpret_cast<char*>(b), 2);
    return ((short) b[1] << 8) + (short) b[0];
}


static CTexture* CreateBMPTexture(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    unsigned char* pixels;

    printf("*** CreateBMPTexture\n");
    in >> fileHeader.b;
    in >> fileHeader.m;
    fileHeader.size = readInt(in);
    fileHeader.reserved = readInt(in);
    fileHeader.offset = readInt(in);

    printf("Checking header . . .\n");
    if (fileHeader.b != 'B' || fileHeader.m != 'M')
        return NULL;
    printf("Header is correct.\n");

    printf("File size: %d\n", fileHeader.size);
    printf("Bytes read: %d\n", in.tellg());

    imageHeader.size = readInt(in);
    imageHeader.width = readInt(in);
    imageHeader.height = readInt(in);
    imageHeader.planes = readShort(in);
    imageHeader.bpp = readShort(in);
    imageHeader.compression = readInt(in);
    imageHeader.imageSize = readInt(in);
    imageHeader.widthPPM = readInt(in);
    imageHeader.heightPPM = readInt(in);
    imageHeader.colorsUsed = readInt(in);
    imageHeader.colorsImportant = readInt(in);

    printf("%d Planes @ %d BPP\n", imageHeader.planes, imageHeader.bpp);
    printf("Size: %d\n", imageHeader.size);
    printf("Dimensions: %d x %d\n", imageHeader.width, imageHeader.height);

    if (imageHeader.width <= 0 || imageHeader.height <= 0)
        return NULL;

    // We currently don't support compressed BMPs
    if (imageHeader.compression != 0)
        return NULL;
    // We don't handle 1-, 2-, or 4-bpp images
    if (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        return NULL;

    printf("Image size: %d\n", imageHeader.imageSize);
    printf("Compression: %d\n", imageHeader.compression);
    printf("WidthPPM x HeightPPM: %d x %d\n", imageHeader.widthPPM, imageHeader.heightPPM);

    unsigned char* palette = NULL;
    if (imageHeader.bpp == 8)
    {
        printf("Reading %d color palette\n", imageHeader.colorsUsed);
        palette = new unsigned char[imageHeader.colorsUsed * 4];
        in.read(reinterpret_cast<char*>(palette), imageHeader.colorsUsed * 4);
    }

    in.seekg(fileHeader.offset, ios::beg);

    unsigned int bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    unsigned int imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new unsigned char[imageBytes];
    in.read(reinterpret_cast<char*>(pixels), imageBytes);

    // check for truncated file

    CTexture* tex = new CTexture(imageHeader.width, imageHeader.height,
                                 GL_RGB);
    if (tex == NULL)
    {
        delete[] pixels;
        return NULL;
    }

    // copy the image into the texture and perform any necessary conversions
    for (int y = 0; y < imageHeader.height; y++)
    {
        unsigned char* src = &pixels[y * bytesPerRow];
        unsigned char* dst = &tex->pixels[y * tex->width * 3];

        switch (imageHeader.bpp)
        {
        case 8:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    unsigned char* color = palette + (*src << 2);
                    dst[0] = color[2];
                    dst[1] = color[1];
                    dst[2] = color[0];
                    src++;
                    dst += 3;
                }
            }
            break;
            
        case 24:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 3;
                    dst += 3;
                }
            }
            break;

        case 32:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 4;
                    dst += 3;
                }
            }
            break;
        }
    }

    delete[] pixels;

    return tex;
}


CTexture* CreateBMPTexture(const char* filename)
{
    ifstream bmpFile(filename, ios::in | ios::binary);

    if (bmpFile.good())
    {
        CTexture* tex = CreateBMPTexture(bmpFile);
        bmpFile.close();
        return tex;
    }
    else
    {
        return NULL;
    }
}


// Helper function for CreateNormalizationCubeMap
static Vec3f cubeVector(int face, float s, float t)
{
    Vec3f v;
    switch (face)
    {
    case 0:
        v = Vec3f(1.0f, -t, -s);
        break;
    case 1:
        v = Vec3f(-1.0f, -t, s);
        break;
    case 2:
        v = Vec3f(s, 1.0f, t);
        break;
    case 3:
        v = Vec3f(s, -1.0f, -t);
        break;
    case 4:
        v = Vec3f(s, -t, 1.0f);
        break;
    case 5:
        v = Vec3f(-s, -t, -1.0f);
        break;
    default:
        // assert(false);
        break;
    }

#if 0
    // Silly test here . . . this produces a normal map with (0, 0, 1) on
    // on the half of the cube on the positive size of the z=0 plane and
    // (0, 0, -1) on the other half.
    //
    // TODO: Experiment with other normal maps as a way to approximate various
    // illumination functions that may be more accurate for planetary rendering
    // than the standard Lambertian model.
    v = Vec3f(0, 0, 1);
    switch (face)
    {
    case 0:
        if (s > 0)
            v = -v;
        break;
    case 1:
        if (s < 0)
            v = -v;
        break;
    case 2:
        if (t < 0)
            v = -v;
        break;
    case 3:
        if (t > 0)
            v = -v;
        break;
    case 4:
        break;
    case 5:
        v = -v;
        break;
    }
#endif

    v.normalize();

    return v;
}


// Build a normalization cube map.  This is used when bump mapping to keep
// the light vector unit length when interpolating.  bindName() need not
// (and must not) be called for a texture created with this method, as the
// name binding stuff all handled right here.
CTexture* CreateNormalizationCubeMap(int size)
{
    // assert(ExtensionSupported("GL_EXT_texture_cube_map"));
    
    CTexture* tex = new CTexture(size, size, GL_RGB);
    if (tex == NULL)
        return NULL;

    glGenTextures(1, &tex->glName);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, tex->glName);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = (float) x / (float) size * 2 - 1;
                float t = (float) y / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                tex->pixels[(y * size + x) * 3]     = 128 + (int) (127 * v.x);
                tex->pixels[(y * size + x) * 3 + 1] = 128 + (int) (127 * v.y);
                tex->pixels[(y * size + x) * 3 + 2] = 128 + (int) (127 * v.z);
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + face,
                     0, GL_RGB8,
                     size, size,
                     0, GL_RGB,
                     GL_UNSIGNED_BYTE,
                     tex->pixels);
    }

    return tex;
}


CTexture* CreateDiffuseLightCubeMap(int size)
{
    // assert(ExtensionSupported("GL_EXT_texture_cube_map"));
    
    CTexture* tex = new CTexture(size, size, GL_RGB);
    if (tex == NULL)
        return NULL;

    GLuint tn;
    glGenTextures(1, &tn);
    glBindTexture(GL_TEXTURE_2D, tn);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = (float) x / (float) size * 2 - 1;
                float t = (float) y / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                float Lz = v.z < 0.0f ? 0.0f : v.z;
                tex->pixels[(y * size + x) * 3]     = (int) (255.99f * Lz);
                tex->pixels[(y * size + x) * 3 + 1] = (int) (255.99f * Lz);
                tex->pixels[(y * size + x) * 3 + 2] = (int) (255.99f * Lz);
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + face,
                     0, GL_RGB8,
                     size, size,
                     0, GL_RGB,
                     GL_UNSIGNED_BYTE,
                     tex->pixels);
    }

    return tex;
}


CTexture* CreateProceduralCubeMap(int size, int format,
                                  ProceduralTexEval func)
{
    CTexture* tex = new CTexture(size, size, format, true);
    if (tex == NULL)
        return NULL;

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = (float) x / (float) size * 2 - 1;
                float t = (float) y / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                func(v.x, v.y, v.z, tex->pixels + ((face * size + y) * size + x) * tex->components);
            }
        }
    }

    return tex;

}
