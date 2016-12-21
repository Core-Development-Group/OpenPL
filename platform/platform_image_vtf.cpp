/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "platform_image.h"

/*  Valve's VTF Format (https://developer.valvesoftware.com/wiki/Valve_Texture_Format)  */

typedef struct VTFHeader {
    PLchar signature[4];    // 'VTF'
    PLuint version[2];      // Minor followed by major.

    PLuint headersize;      // I guess this is used to support header alterations?

    PLushort width, height; // Width and height of the texture.

    PLint flags;

    PLushort frames;        // For animated texture sets.
    PLushort firstframe;    // Initial frame to start from.

    PLuchar padding0[4];

    PLfloat reflectivity[3];

    PLuchar padding1[4];

    PLfloat bumpmapscale;

    PLuint highresimageformat;

    PLuchar mipmaps;

    PLuint lowresimageformat;
    PLuchar lowresimagewidth;
    PLuchar lowresimageheight;
} VTFHeader;

typedef struct VTFHeader72 {
    PLushort depth;
} VTFHeader72;

typedef struct VTFHeader73 {
    PLuchar padding2[3];
    PLuint numresources;
} VTFHeader73;

#define VTF_VERSION_MAJOR   7
#define VTF_VERSION_MINOR   5

enum VTFFlag {
    VTF_FLAG_POINTSAMPLE = 0x00000001,
    VTF_FLAG_TRILINEAR = 0x00000002,
    VTF_FLAG_CLAMPS = 0x00000004,
    VTF_FLAG_CLAMPT = 0x00000008,
    VTF_FLAG_ANISOTROPIC = 0x00000010,
    VTF_FLAG_HINT_DXT5 = 0x00000020,
    VTF_FLAG_NO_COMPRESS = 0x00000040,
    VTF_FLAG_NORMAL = 0x00000080,
    VTF_FLAG_NOMIP = 0x00000100,
    VTF_FLAG_NOLOD = 0x00000200,
    VTF_FLAG_ALL_MIPS = 0x00000400,
    VTF_FLAG_PROCEDURAL = 0x00000800,
    VTF_FLAG_ONEBITALPHA = 0x00001000, // Automatically generated by vtex from the texture data.
    VTF_FLAG_EIGHTBITALPHA = 0x00002000, // Automatically generated by vtex from the texture data.
    VTF_FLAG_ENVMAP = 0x00004000,
    VTF_FLAG_RENDERTARGET = 0x00008000,
    VTF_FLAG_DEPTHRENDERTARGET = 0x00010000,
    VTF_FLAG_NODEBUGOVERRIDE = 0x00020000,
    VTF_FLAG_SINGLECOPY = 0x00040000,
    VTF_FLAG_PRE_SRGB = 0x00080000,
    VTF_FLAG_PREMULTIPLY = 0x00100000,
    VTF_FLAG_DUDV = 0x00200000,
    VTF_FLAG_ALPHATESTMIPMAP = 0x00400000,
    VTF_FLAG_NODEPTHBUFFER = 0x00800000,
    VTF_FLAG_UNUSED_01000000 = 0x01000000,
    VTF_FLAG_CLAMPU = 0x02000000,
    VTF_FLAG_VERTEXTEXTURE = 0x04000000,
    VTF_FLAG_SSBUMP = 0x08000000,
    VTF_FLAG_UNUSED_10000000 = 0x10000000,
    VTF_FLAG_BORDER = 0x20000000,
} VTFFlag;

enum VTFFace {
    VTF_FACE_RIGHT,
    VTF_FACE_LEFT,
    VTF_FACE_BACK,
    VTF_FACE_FRONT,
    VTF_FACE_UP,
    VTF_FACE_DOWN,

    VTF_FACE_SPHEREMAP
} VTFFace;

enum VTFFormat {
    VTF_FORMAT_RGBA8888,
    VTF_FORMAT_ABGR8888,
    VTF_FORMAT_RGB888,
    VTF_FORMAT_BGR888,
    VTF_FORMAT_RGB565,
    VTF_FORMAT_I8,
    VTF_FORMAT_IA88,
    VTF_FORMAT_P8,
    VTF_FORMAT_A8,
    VTF_FORMAT_RGB888_BLUESCREEN,
    VTF_FORMAT_BGR888_BLUESCREEN,
    VTF_FORMAT_ARGB8888,
    VTF_FORMAT_BGRA8888,
    VTF_FORMAT_DXT1,
    VTF_FORMAT_DXT3,
    VTF_FORMAT_DXT5,
    VTF_FORMAT_BGRX8888,
    VTF_FORMAT_BGR565,
    VTF_FORMAT_BGRX5551,
    VTF_FORMAT_BGRA4444,
    VTF_FORMAT_DXT1_ONEBITALPHA,
    VTF_FORMAT_BGRA5551,
    VTF_FORMAT_UV88,
    VTF_FORMAT_UVWQ8888,
    VTF_FORMAT_RGBA16161616F,
    VTF_FORMAT_RGBA16161616,
    VTF_FORMAT_UVLX8888
} VTFFormat;

void _plConvertVTFFormat(PLImage *image, PLuint in) {
    switch(in) {
        case VTF_FORMAT_A8:
            image->format = PL_IMAGEFORMAT_RGB4;
            image->colour_format = PL_COLOURFORMAT_RGB;
            break;
        case VTF_FORMAT_ABGR8888:
            image->format = PL_IMAGEFORMAT_RGBA8;
            image->colour_format = PL_COLOURFORMAT_ABGR;
            break;
        case VTF_FORMAT_ARGB8888:
            image->format = PL_IMAGEFORMAT_RGBA8;
            image->colour_format = PL_COLOURFORMAT_ARGB;
            break;
        case VTF_FORMAT_BGR565:
            image->format = PL_IMAGEFORMAT_RGB565;
            image->colour_format = PL_COLOURFORMAT_BGR;
            break;
        case VTF_FORMAT_BGR888:
        case VTF_FORMAT_BGR888_BLUESCREEN:
            image->format = PL_IMAGEFORMAT_RGB8;
            image->colour_format = PL_COLOURFORMAT_BGR;
            break;
        case VTF_FORMAT_BGRA4444:
            image->format = PL_IMAGEFORMAT_RGBA4;
            image->colour_format = PL_COLOURFORMAT_BGRA;
            break;
        case VTF_FORMAT_BGRA5551:
            image->format = PL_IMAGEFORMAT_RGB5A1;
            image->colour_format = PL_COLOURFORMAT_BGRA;
            break;
        case VTF_FORMAT_BGRA8888:
        case VTF_FORMAT_BGRX8888:
            image->format = PL_IMAGEFORMAT_RGBA8;
            image->colour_format = PL_COLOURFORMAT_BGRA;
            break;
        case VTF_FORMAT_DXT1:
            image->format = PL_IMAGEFORMAT_RGB_DXT1;
            image->colour_format = PL_COLOURFORMAT_RGB;
            break;
        case VTF_FORMAT_DXT1_ONEBITALPHA:
            image->format = PL_IMAGEFORMAT_RGBA_DXT1;
            image->colour_format = PL_COLOURFORMAT_RGBA;
            break;
        case VTF_FORMAT_DXT3:
            image->format = PL_IMAGEFORMAT_RGBA_DXT3;
            image->colour_format = PL_COLOURFORMAT_RGBA;
            break;
        case VTF_FORMAT_DXT5:
            image->format = PL_IMAGEFORMAT_RGBA_DXT5;
            image->colour_format = PL_COLOURFORMAT_RGBA;
            break;
        case VTF_FORMAT_I8:                 abort();    // todo
        case VTF_FORMAT_IA88:               abort();    // todo
        case VTF_FORMAT_P8:                 abort();    // todo
        case VTF_FORMAT_RGB565:             abort();    // todo
        case VTF_FORMAT_RGB888:             // Same as RGB888_BLUESCREEN.
        case VTF_FORMAT_RGB888_BLUESCREEN:
            image->format = PL_IMAGEFORMAT_RGB8;
            image->colour_format = PL_COLOURFORMAT_RGB;
            break;
        case VTF_FORMAT_RGBA8888:
            image->format = PL_IMAGEFORMAT_RGBA8;
            image->colour_format = PL_COLOURFORMAT_RGBA;
            break;
        case VTF_FORMAT_RGBA16161616:
            image->format = PL_IMAGEFORMAT_RGBA16;
            image->colour_format = PL_COLOURFORMAT_RGBA;
        case VTF_FORMAT_RGBA16161616F:
            image->format = PL_IMAGEFORMAT_RGBA16F;
            image->colour_format = PL_COLOURFORMAT_RGBA;
            break;
        case VTF_FORMAT_UV88:               abort();    // todo
        case VTF_FORMAT_UVLX8888:           abort();    // todo
        case VTF_FORMAT_UVWQ8888:           abort();    // todo
        default:
            image->format = PL_IMAGEFORMAT_UNKNOWN;
            image->colour_format = PL_COLOURFORMAT_RGB;
    }
}

PLresult plLoadVTFImage(FILE *fin, PLImage *out) {
    plFunctionStart();

    VTFHeader header;
    memset(&header, 0, sizeof(header));
#define VTF_VERSION(maj, min)   ((maj == header.version[1] && min <= header.version[0]) || maj < header.version[0])

    if (fread(&header, sizeof(VTFHeader), 1, fin) != 1)
        return PL_RESULT_FILEREAD;
    else if((header.signature[0] != 'V') || (header.signature[1] != 'T') || (header.signature[2] != 'F') ||
            (header.signature[3] != 0))
        return PL_RESULT_FILETYPE;
    else if (VTF_VERSION(7, 5))
        return PL_RESULT_FILEVERSION;

    else if (!plIsValidImageSize(header.width, header.height))
        return PL_RESULT_IMAGERESOLUTION;
    else if(header.lowresimagewidth && header.lowresimageheight) {
        if ((header.lowresimagewidth > 16) || (header.lowresimageheight > 16) ||
            (header.lowresimagewidth > header.width) || (header.lowresimageheight > header.height))
            return PL_RESULT_IMAGERESOLUTION;

        if(header.lowresimageformat != VTF_FORMAT_DXT1)
            return PL_RESULT_IMAGEFORMAT;
    }

    // todo, use the headersize flag so we can load this more intelligently!

    VTFHeader72 header2;
    if (header.version[1] >= 2) {
        memset(&header2, 0, sizeof(header));
        if (fread(&header2, sizeof(VTFHeader72), 1, fin) != 1)
            return PL_RESULT_FILEREAD;
    }
    VTFHeader73 header3;
    if (header.version[1] >= 3) {
        memset(&header3, 0, sizeof(header));
        if (fread(&header3, sizeof(VTFHeader73), 1, fin) != 1)
            return PL_RESULT_FILEREAD;
    }

    memset(out, 0, sizeof(PLImage));

    out->width = header.width;
    out->height = header.height;

    _plConvertVTFFormat(out, header.highresimageformat);

    out->levels = header.mipmaps;
    out->data = new PLbyte*[out->levels];

    /*
    if (header.version[1] >= 3) {
        for (PLuint i = 0; i < header3.numresources; i++) {
            // todo, support for later VTF versions.
        }
    } else */ {
        PLuint faces = 1;
        if(header.flags & VTF_FLAG_ENVMAP)
            faces = 6;

        // VTF's typically include a tiny thumbnail image at the start, which we'll skip.
        fseek(fin, header.lowresimagewidth * header.lowresimageheight / 2, SEEK_CUR);
        for (PLuint mipmap = 0; mipmap < header.mipmaps; mipmap++) {

#if 0 // skip frames and faces for now...
            for(PLuint frame = 0; frame < header.frames; frame++) {
                for(PLuint face = 0; face < faces; face++) {

                }
            }
#endif

            /* Need to scale mipsize depending on depth of this? Surely each one will use less and less data???
             * Looking at the DevIL implementation, this doesn't seem like something they worried about but I'm
             * still slightly concerned about it here.
             */

            PLuint mipsize = _plGetImageSize(out->format, 0, 0);
            out->data[mipmap] = new PLbyte[mipsize];
            if(fread(out->data[mipmap], sizeof(PLbyte), mipsize, fin) != mipsize) {
                delete[] out->data;
                return PL_RESULT_FILEREAD;
            }

            // Skip any additional frames.
            fseek(fin, mipsize * header.frames * faces, SEEK_CUR);
        }
    }

    plFunctionEnd();
}