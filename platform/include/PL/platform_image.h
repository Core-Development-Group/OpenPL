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
#pragma once

#include "platform.h"

typedef enum PLImageFormat {
    PL_IMAGEFORMAT_UNKNOWN,

    PL_IMAGEFORMAT_RGB4,      // 4 4 4 0
    PL_IMAGEFORMAT_RGBA4,     // 4 4 4 4
    PL_IMAGEFORMAT_RGB5,      // 5 5 5 0
    PL_IMAGEFORMAT_RGB5A1,    // 5 5 5 1
    PL_IMAGEFORMAT_RGB565,    // 5 6 5 0
    PL_IMAGEFORMAT_RGB8,      // 8 8 8 0
    PL_IMAGEFORMAT_RGBA8,     // 8 8 8 8
    PL_IMAGEFORMAT_RGBA12,    // 12 12 12 12
    PL_IMAGEFORMAT_RGBA16,    // 16 16 16 16
    PL_IMAGEFORMAT_RGBA16F,   // 16 16 16 16

    PL_IMAGEFORMAT_RGBA_DXT1,
    PL_IMAGEFORMAT_RGB_DXT1,
    PL_IMAGEFORMAT_RGBA_DXT3,
    PL_IMAGEFORMAT_RGBA_DXT5,

    PL_IMAGEFORMAT_RGB_FXT1
} PLImageFormat;

typedef enum PLColourFormat {
    PL_COLOURFORMAT_ARGB,
    PL_COLOURFORMAT_ABGR,
    PL_COLOURFORMAT_RGB,
    PL_COLOURFORMAT_BGR,
    PL_COLOURFORMAT_RGBA,
    PL_COLOURFORMAT_BGRA,
} PLColourFormat;

typedef struct PLImage {
#if 1
    uint8_t **data;
#else
    uint8_t *data;
#endif

    unsigned int x, y;
    unsigned int width, height;
    unsigned int size;
    unsigned int levels;

    char path[PL_SYSTEM_MAX_PATH];

    PLImageFormat format;
    PLColourFormat colour_format;

    unsigned int flags;
} PLImage;

#define PLIMAGE_EXTENSION_FTX   "ftx"   // Ritual's FTX image format
#define PLIMAGE_EXTENSION_DTX   "dtx"   // Direct Texture (LithTech)
#define PLIMAGE_EXTENSION_PPM   "ppm"   // Portable Pixel Map
#define PLIMAGE_EXTENSION_KTX   "ktx"
#define PLIMAGE_EXTENSION_TGA   "tga"
#define PLIMAGE_EXTENSION_PNG   "png"
#define PLIMAGE_EXTENSION_DDS   "dds"
#define PLIMAGE_EXTENSION_VTF   "vtf"   // Valve Texture Format (Source Engine)

PL_EXTERN_C



PL_EXTERN bool plLoadImage(const char *path, PLImage *out);
PL_EXTERN PLresult plLoadImagef(FILE *fin, const char *path, PLImage *out);
PL_EXTERN PLresult plWriteImage(const PLImage *image, const char *path);

PL_EXTERN unsigned int plGetSamplesPerPixel(PLColourFormat format);

PL_EXTERN bool plIsValidImageSize(unsigned int width, unsigned int height);
PL_EXTERN bool plIsCompressedImageFormat(PLImageFormat format);

#if defined(PL_INTERNAL)

void plFreeImage(PLImage *image);

unsigned int _plGetImageSize(PLImageFormat format, unsigned int width, unsigned int height);

bool DDSFormatCheck(FILE *fin);
bool _plDTXFormatCheck(FILE *fin);
bool _plVTFFormatCheck(FILE *fin);
bool TIMFormatCheck(FILE *fin);
bool _plBMPFormatCheck(FILE *fin);

PLresult _plLoadFTXImage(FILE *fin, PLImage *out);           // Ritual's FTX image format.
PLresult _plLoadPPMImage(FILE *fin, PLImage *out);           // Portable Pixel Map format.
PLresult _plLoadDTXImage(FILE *fin, PLImage *out);           // Lithtech's DTX image format.
PLresult _plLoadVTFImage(FILE *fin, PLImage *out);           // Valve's VTF image format.
PLresult LoadDDSImage(FILE *fin, PLImage *out);
PLresult LoadTIMImage(FILE *fin, PLImage *out);
PLresult _plLoadBMPImage(FILE *fin, PLImage *out);

#endif

PLresult plWriteTIFFImage(const PLImage *image, const char *path);

PL_EXTERN_C_END