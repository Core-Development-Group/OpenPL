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
#include "platform_math.h"
#include "platform_graphics.h"

enum {
    PL_MODELTYPE_STATIC,
    PL_MODELTYPE_ANIMATED,
    PL_MODELTYPE_SKELETAL
};

#define PLMODEL_MAX_MESHES   32

typedef struct PLModelFrame {
    PLTriangle  *triangles;
    PLVertex    *vertices;

    PLVector3D mins, maxs; // Bounds
} PLModelFrame;

// Static mesh.
typedef struct PLStaticModel {
    unsigned int num_triangles;
    unsigned int num_vertices;

    PLMesh mesh[PLMODEL_MAX_MESHES];

    PLBBox3D bounds;

    PLModelFrame frame; // todo, remove! Obsolete by above!
} PLStaticModel;

// Per-vertex animated mesh.
typedef struct PLAnimatedModel {
    unsigned int num_triangles;
    unsigned int num_vertices;
    unsigned int num_frames;

    PLMeshPrimitive primitive;

    PLModelFrame *frames;
} PLAnimatedModel;

// Mesh with bone structure.
typedef struct PLSkeletalModel {
    unsigned int num_triangles;
    unsigned int num_vertices;

    PLMeshPrimitive primitive;

    // Unfinished...
} PLSkeletalModel;

#include "platform_model_u3d.h"
#include "platform_model_obj.h"

PL_EXTERN_C

// Static
PLStaticModel *plCreateStaticModel(void);
PLStaticModel *plLoadStaticModel(const char *path);
void plDeleteStaticModel(PLStaticModel *model);

// Animated
PLAnimatedModel *plCreateAnimatedModel(void);
PLAnimatedModel *plLoadAnimatedModel(const char *path);
void plDeleteAnimatedModel(PLAnimatedModel *model);

PLAnimatedModel *plLoadU3DModel(const char *path);

void plDrawStaticModel(PLStaticModel *model);

// Utility
void plGenerateStaticModelNormals(PLStaticModel *model);
void plGenerateAnimatedModelNormals(PLAnimatedModel *model);
void plGenerateSkeletalModelNormals(PLSkeletalModel *model);

PLVector3D plGenerateVertexNormal(PLVector3D a, PLVector3D b, PLVector3D c);

PL_EXTERN_C_END
