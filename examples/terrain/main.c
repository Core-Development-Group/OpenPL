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

#include <platform_log.h>
#include <platform_window.h>
#include <platform_graphics.h>
#include <platform_graphics_mesh.h>
#include <platform_filesystem.h>

#include <GLFW/glfw3.h>
#include <platform_model.h>

#define TITLE "H0G Loader"
#define LOG "hog_loader"
#define PRINT(...) printf(__VA_ARGS__); plWriteLog(LOG, __VA_ARGS__)
#define WIDTH 1024
#define HEIGHT 768

/*  VTX Format Specification    */

typedef struct VTXCoord {
    int16_t x;
    int16_t y;
    int16_t z;
    uint16_t bone_index;
} VTXCoord;

/*  FAC Format Specification    */

typedef struct FACHeader {
    uint32_t padding[4];    // This is always blank
    uint32_t num_triangles;    // Number of FACBlocks
    uint32_t unknown3;
} FACHeader;

// ?   I0   I1  I2   N0  N1   N2  ?    TI  P    ?   ?    ?   ?    S   T
// 01000F00 0E001300 0F000E00 13003420 50000000 30203134 00002031 001F001F
// 01001000 0F001300 10000F00 13003420 50000000 30203331 00003432 001F001F
// 01001100 10001300 11001000 13003120 50000000 36203331 00003220 021F001F
// 01001200 11001300 12001100 1300390D 50000000 36203333 00003220 031F021F

typedef struct __attribute__((packed)) FACTriangle {
    uint16_t unknown1;

    uint16_t indices[3];    // Vertex indices
    uint16_t normal[3];     // Normals
    uint16_t unknown11;     // ??
    uint16_t texture_index; // Matches TIM listed in MTD package.

    uint16_t padding;

    int32_t unknown2;
    int32_t unknown3;

    int16_t ST[2];
} FACTriangle;

typedef struct __attribute__((packed)) FACQuad {

    uint32_t unknown4;
    uint32_t unknown5;

    uint16_t indices[4];    // Vertex indices

    uint16_t normal[4];     // Normals

    uint16_t texture_index; // Matches TIM listed in MTD package.

    uint16_t padding;

    uint16_t unknown3;
    int32_t unknown6;
    int16_t unknown7;
} FACQuad;

/*  NO2 Format Specification    */
/* The NO2 format is used by Hogs of War to store pre-calculated normals
 * for the mesh, as far as we've determined.
 */

typedef struct NO2Index { // todo
    // D2C7003F 7EE456BD 38D75CBF 00004041

    int32_t unknown0;
    int32_t unknown1;
    int32_t unknown2;
    int32_t unknown3;
} NO2Index;

/*  HIR Format Specification    */
/* Used to store our piggy bones.
 */

// P        X   Y    Z   ?    ?   ?    ?   ?

// 00000000 0000EBFF 01000000 00000000 00000000
// 01000000 06004FFF 02000000 00000000 00000000
// 01000000 FDFF9BFF 58000000 00000000 00000000
// 03000000 03000000 6E000000 00000000 00000000
// 04000000 FDFF0000 6F000000 00000000 00000000
// 01000000 FEFF9BFF A9FF0000 00000000 00000000

typedef struct __attribute__((packed)) HIRBone {
    uint32_t parent;

    int16_t coords[3];
    int16_t padding;

    int32_t unknown0;
    int32_t unknown1;
} HIRBone;

/*  SRL Format Specification        */
/* The SRL format is used as an index for sounds used
 * by the game. It's just a text-based format, nothing
 * particular special.
 */

void load_srl_file(const char *path) {
    // todo, parse srl file format
    // 099 < number of sounds in the document
    // 000 < ???
    // 000 < ???
}

/*  MAD/MTD Format Specification    */
/* The MAD/MTD format is the package format used by
 * Hogs of War to store and index content used by
 * the game.
 *
 * Files within these packages are expected to be in
 * a specific order, as both the game and other assets
 * within the game rely on this order so that they, for
 * example, will know which textures to load in / use.
 *
 * Because of this, any package that's recreated will need
 * to be done so in a way that preserves the original file
 * order.
 *
 * Thanks to solemnwarning for his help on this one!
 */

typedef struct __attribute__((packed)) MADIndex {
    // F                          P        O
    // 65796573 3030312E 74696D00 00000000 50010000 8C020000

    char file[12];

    int32_t padding0;

    uint32_t offset;
    uint32_t length;
} MADIndex;

bool check_mad_file_name(const char *path) {
    for(unsigned int i = 0; i < 12; i++) {
        if(!isalpha(path[i] && !isnumber(path[i]))) {
            return false;
        }
    }

    return true;
}

void extract_mad_file(const char *path) {
    PRINT("\nOpening %s\n", path);

    char outpath[PL_SYSTEM_MAX_PATH] = { 0 };
    plStripExtension(outpath, path);
    snprintf(outpath, sizeof(outpath), "./%s", outpath);
    if(!plCreateDirectory(outpath)) {
        PRINT("Failed to create directory!\n%s\n", plGetError());
        return;
    }

    FILE *file = fopen(path, "rb");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return;
    }

    unsigned int lowest_offset = UINT32_MAX; fpos_t position;
    do {
        MADIndex index;
        if(fread(&index, sizeof(MADIndex), 1, file) != 1) {
            PRINT("Invalid index size!\n");
            break;
        }

        if(lowest_offset > index.offset) {
            lowest_offset = index.offset;
        }

        char foutpath[PL_SYSTEM_MAX_PATH];
        snprintf(foutpath, sizeof(foutpath), "%s/%s", outpath, index.file);
        if(plFileExists(foutpath)) {
            continue;
        }

        PRINT("    Exporting %s...", index.file);

        fgetpos(file, &position);

        fseek(file, index.offset, SEEK_SET);
        uint8_t *data = calloc(index.length, sizeof(uint8_t));
        if(fread(data, sizeof(uint8_t), index.length, file) == index.length) {
            FILE *out = fopen(foutpath, "wb");
            if(out && fwrite(data, sizeof(uint8_t), index.length, out) == index.length) {
                PRINT("Done!\n");
            } else {
                PRINT("Failed!\n");
            }
            fclose(out);
        }

        fsetpos(file, &position);

    } while(position < lowest_offset);

    fclose(file);
}

//////////////////////////////////////////////////////

#define MAX_VERTICES    2048
#define MAX_TRIANGLES   4096
#define MAX_QUADS       4096
#define MAX_BONES       32

typedef struct PIGModel {
    VTXCoord coords[MAX_VERTICES];
    HIRBone bones[MAX_BONES];
    FACTriangle triangles[MAX_TRIANGLES];
    FACQuad quads[MAX_QUADS];

    unsigned int num_vertices;
    unsigned int num_triangles; // triangles * (quads * 2)
    unsigned int num_bones;

    PLMesh *tri_mesh;       // Our actual output!
    PLMesh *skeleton_mesh;  // preview of skeleton
    PLMesh *vertex_mesh;    // preview of vertices

    PLVector3D angles;
    PLVector3D position;
} PIGModel;

PIGModel model;

void load_hir_file(const char *path) {
    PRINT("\nOpening %s\n", path);

    FILE *file = fopen(path, "r");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return;
    }

    memset(model.bones, 0, sizeof(HIRBone) * MAX_BONES);
    model.num_bones = (unsigned int) fread(model.bones, sizeof(HIRBone), 32, file);
    if(model.num_bones > MAX_BONES) {
        PRINT("Unexpected number of bones, greater than 32! (%d)\n", model.num_bones);
        model.num_bones = MAX_BONES;
    }

    model.skeleton_mesh = plCreateMesh(
            PL_PRIMITIVE_POINTS,
            PL_DRAW_IMMEDIATE,
            0,
            model.num_bones * 2
    );
    for(unsigned int i = 0; i < model.num_bones; i++) {
#if 1
        PRINT("BONE %d\n", i);
        PRINT("    parent(%d)\n", model.bones[i].parent);
        PRINT("    coords(%d %d %d)\n",
              model.bones[i].coords[0],
              model.bones[i].coords[1],
              model.bones[i].coords[2]);
#endif

#if 0
        plSetMeshVertexPosition3f(model.skeleton_mesh, i,
                                  model.bones[model.bones[i].parent].coords[0],
                                  model.bones[model.bones[i].parent].coords[1],
                                  model.bones[model.bones[i].parent].coords[2]);
        plSetMeshVertexColour(model.skeleton_mesh, i, plCreateColour4b(PL_COLOUR_RED));
#endif

        plSetMeshVertexPosition3f(model.skeleton_mesh, i,
                                  model.bones[i].coords[0],
                                  model.bones[i].coords[1],
                                  model.bones[i].coords[2]);
        plSetMeshVertexColour(model.skeleton_mesh, i, plCreateColour4b(PL_COLOUR_RED));

    }

    fclose(file);
}

void load_fac_file(const char *path) {
    FACHeader header;
    memset(&header, 0, sizeof(FACHeader));

    PRINT("\nOpening %s\n", path);

    FILE *file = fopen(path, "r");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return;
    }

    if(fread(&header, sizeof(FACHeader), 1, file) != 1) {
        PRINT("Invalid file header...\n");
        fclose(file);
        return;
    }

    for(int i = 0; i < plArrayElements(header.padding); i++) {
        if(header.padding[0] != 0) {
            PRINT("Invalid FAC file!\n");
            fclose(file);
            return;
        }
    }

    PRINT("triangles: %d\n", header.num_triangles);
    FACTriangle triangles[header.num_triangles];
    if(header.num_triangles != 0) {
        if(fread(triangles, sizeof(FACTriangle), header.num_triangles, file) != header.num_triangles) {
            PRINT("Unexpected block size!\n");
            goto CLEANUP;
        }
#if 0
        for(unsigned int i = 0; i < header.num_triangles; i++) {
            PRINT("TRIANGLE %d\n", i);
            PRINT("    indices(%d %d %d)\n", triangles[i].indices[0], triangles[i].indices[1], triangles[i].indices[2]);
            PRINT("    normals(%d %d %d)\n", triangles[i].normal[0], triangles[i].normal[1], triangles[i].normal[2]);
            PRINT("    texture index(%d)\n", triangles[i].texture_index);
            PRINT("    texture coords(%d %d)\n", triangles[i].ST[0], triangles[i].ST[1]);
        }
#endif
    }

    FACQuad quads[2048];
    memset(quads, 0, sizeof(FACQuad));
    unsigned int num_quads = (unsigned int) fread(quads, sizeof(FACQuad), 2048, file);
    PRINT("\nquads: %d\n\n", model.num_triangles);
#if 0
    for(unsigned int i = 0; i < num_quads; i++) {
        PRINT("QUAD %d\n", i);
        PRINT("    indices(%d %d %d %d)\n",
              quads[i].indices[0], quads[i].indices[1], quads[i].indices[2], quads[i].indices[3]);
        PRINT("    normals(%d %d %d %d)\n",
              quads[i].normal[0], quads[i].normal[1], quads[i].normal[2], quads[i].normal[3]);
        PRINT("    texture index(%d)\n", quads[i].texture_index);
    }
#endif

    model.num_triangles = header.num_triangles + (num_quads * 2);
    model.num_vertices = model.num_triangles * 3;

    model.tri_mesh = plCreateMesh(
            PL_PRIMITIVE_TRIANGLES,
            PL_DRAW_IMMEDIATE,
            model.num_triangles,
            model.num_vertices
    );
    unsigned int cur_vert = 0; uint8_t r, g, b;
    PLVector3D normal;
    for(unsigned int i = 0; i < header.num_triangles; i++, cur_vert++) {

        PLVector3D va = plCreateVector3D(model.coords[triangles[i].indices[0]].x,
                                         model.coords[triangles[i].indices[0]].y,
                                         model.coords[triangles[i].indices[0]].z);
        PLVector3D vb = plCreateVector3D(model.coords[triangles[i].indices[1]].x,
                                         model.coords[triangles[i].indices[1]].y,
                                         model.coords[triangles[i].indices[1]].z);
        PLVector3D vc = plCreateVector3D(model.coords[triangles[i].indices[2]].x,
                                         model.coords[triangles[i].indices[2]].y,
                                         model.coords[triangles[i].indices[2]].z);
        normal = plGenerateVertexNormal(va, vb, vc);

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[triangles[i].indices[0]].x +
                                  model.bones[model.coords[triangles[i].indices[0]].bone_index].coords[0],
                                  model.coords[triangles[i].indices[0]].y +
                                  model.bones[model.coords[triangles[i].indices[0]].bone_index].coords[1],
                                  model.coords[triangles[i].indices[0]].z +
                                  model.bones[model.coords[triangles[i].indices[0]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[triangles[i].indices[0]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[triangles[i].indices[1]].x +
                                  model.bones[model.coords[triangles[i].indices[1]].bone_index].coords[0],
                                  model.coords[triangles[i].indices[1]].y +
                                  model.bones[model.coords[triangles[i].indices[1]].bone_index].coords[1],
                                  model.coords[triangles[i].indices[1]].z +
                                  model.bones[model.coords[triangles[i].indices[1]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[triangles[i].indices[1]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[triangles[i].indices[2]].x +
                                  model.bones[model.coords[triangles[i].indices[2]].bone_index].coords[0],
                                  model.coords[triangles[i].indices[2]].y +
                                  model.bones[model.coords[triangles[i].indices[2]].bone_index].coords[1],
                                  model.coords[triangles[i].indices[2]].z +
                                  model.bones[model.coords[triangles[i].indices[2]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[triangles[i].indices[2]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));

#if 0
        PRINT(" %d 0(%d %d %d) 1(%d %d %d) 2(%d %d %d)\n",
              i,

              model.coords[triangles[i].indices[0]].x,
              model.coords[triangles[i].indices[0]].y,
              model.coords[triangles[i].indices[0]].z,

              model.coords[triangles[i].indices[1]].x,
              model.coords[triangles[i].indices[1]].y,
              model.coords[triangles[i].indices[1]].z,

              model.coords[triangles[i].indices[2]].x,
              model.coords[triangles[i].indices[2]].y,
              model.coords[triangles[i].indices[2]].z
        );
#endif
    }

#if 1
    for(unsigned int i = 0; i < num_quads; i++, cur_vert++) {

        PLVector3D va = plCreateVector3D(model.coords[quads[i].normal[0]].x,
                                        model.coords[quads[i].normal[0]].y,
                                        model.coords[quads[i].normal[0]].z);
        PLVector3D vb = plCreateVector3D(model.coords[quads[i].normal[1]].x,
                                        model.coords[quads[i].normal[1]].y,
                                        model.coords[quads[i].normal[1]].z);
        PLVector3D vc = plCreateVector3D(model.coords[quads[i].normal[2]].x,
                                        model.coords[quads[i].normal[2]].y,
                                        model.coords[quads[i].normal[2]].z);
        normal = plGenerateVertexNormal(va, vb, vc);

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[0]].x +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[0],
                                  model.coords[quads[i].indices[0]].y +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[1],
                                  model.coords[quads[i].indices[0]].z +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[0]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[1]].x +
                                  model.bones[model.coords[quads[i].indices[1]].bone_index].coords[0],
                                  model.coords[quads[i].indices[1]].y +
                                  model.bones[model.coords[quads[i].indices[1]].bone_index].coords[1],
                                  model.coords[quads[i].indices[1]].z +
                                  model.bones[model.coords[quads[i].indices[1]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[1]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[2]].x +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[0],
                                  model.coords[quads[i].indices[2]].y +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[1],
                                  model.coords[quads[i].indices[2]].z +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[2]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        va = plCreateVector3D(model.coords[quads[i].normal[2]].x,
                                         model.coords[quads[i].normal[2]].y,
                                         model.coords[quads[i].normal[2]].z);
        vb = plCreateVector3D(model.coords[quads[i].normal[3]].x,
                                         model.coords[quads[i].normal[3]].y,
                                         model.coords[quads[i].normal[3]].z);
        vc = plCreateVector3D(model.coords[quads[i].normal[0]].x,
                                         model.coords[quads[i].normal[0]].y,
                                         model.coords[quads[i].normal[0]].z);
        normal = plGenerateVertexNormal(va, vb, vc);

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[2]].x +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[0],
                                  model.coords[quads[i].indices[2]].y +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[1],
                                  model.coords[quads[i].indices[2]].z +
                                  model.bones[model.coords[quads[i].indices[2]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[2]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[3]].x +
                                  model.bones[model.coords[quads[i].indices[3]].bone_index].coords[0],
                                  model.coords[quads[i].indices[3]].y +
                                  model.bones[model.coords[quads[i].indices[3]].bone_index].coords[1],
                                  model.coords[quads[i].indices[3]].z +
                                  model.bones[model.coords[quads[i].indices[3]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[3]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
        cur_vert++;

        plSetMeshVertexPosition3f(model.tri_mesh, cur_vert,
                                  model.coords[quads[i].indices[0]].x +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[0],
                                  model.coords[quads[i].indices[0]].y +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[1],
                                  model.coords[quads[i].indices[0]].z +
                                  model.bones[model.coords[quads[i].indices[0]].bone_index].coords[2]
        );
        plSetMeshVertexNormal3f(model.tri_mesh, cur_vert, normal.x, normal.y, normal.z);
        srand(model.coords[quads[i].indices[0]].bone_index);
        r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
        plSetMeshVertexColour(model.tri_mesh, cur_vert, plCreateColour4b(r, g, b, 255));
    }
#endif
    plUploadMesh(model.tri_mesh);

    CLEANUP:
    fclose(file);
}

void upload_vtx(void) {
    model.vertex_mesh = plCreateMesh(
            PL_PRIMITIVE_POINTS,
            PL_DRAW_IMMEDIATE,
            0,
            model.num_vertices
    );

    for(unsigned int i = 0; i < model.num_vertices; i++) {
#if 0
        PRINT("VERTEX %d\n", i);
        PRINT("    position(%d %d %d)\n", model.coords[i].x, model.coords[i].y, model.coords[i].z);
        PRINT("    bone(%d(%d %d %d))\n", model.coords[i].bone_index,
              model.bones[model.coords[i].bone_index].coords[0],
              model.bones[model.coords[i].bone_index].coords[1],
              model.bones[model.coords[i].bone_index].coords[2]
        );
#endif

        plSetMeshVertexPosition3f(model.vertex_mesh, i, model.coords[i].x, model.coords[i].y, model.coords[i].z);
        if(model.coords[i].bone_index > 0) {
            srand(model.coords[i].bone_index);
            uint8_t r = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
            uint8_t g = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
            uint8_t b = (uint8_t)((rand() % 255) * ((rand() % 254) + 1));
            plSetMeshVertexColour(model.vertex_mesh, i, plCreateColour4b(r, g, b, 255));
        } else {
            plSetMeshVertexColour(model.vertex_mesh, i, plCreateColour4b(PL_COLOUR_RED));
        }
    }

    plUploadMesh(model.vertex_mesh);
}

void load_vtx_file(const char *path) {
    PRINT("\nOpening %s\n", path);

    FILE *file = fopen(path, "r");
    if(!file) {
        PRINT("Failed to load file %s!\n", path);
        return;
    }

    model.num_vertices = (unsigned int) fread(model.coords, sizeof(VTXCoord), 2048, file);
    if(!model.num_vertices) {
        PRINT("Empty model!\n");
        fclose(file);
        return;
    }

    PRINT("Vertices: %d\n", model.num_vertices);

    upload_vtx();
}

enum {
    VIEW_MODE_LIT,
    VIEW_MODE_WIREFRAME,
    VIEW_MODE_POINTS,
    VIEW_MODE_WEIGHTS,
    VIEW_MODE_SKELETON
};
int view_mode = VIEW_MODE_LIT;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {

    switch(key) {
        default: break;

        case GLFW_KEY_1: {
            if(action == GLFW_PRESS) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                view_mode = VIEW_MODE_LIT;
            }
            break;
        }
        case GLFW_KEY_2: {
            if((action == GLFW_PRESS) && (mode != VIEW_MODE_WIREFRAME)) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDisable(GL_LIGHTING);
                view_mode = VIEW_MODE_WIREFRAME;
            }
            break;
        }
        case GLFW_KEY_3: {
            if((action == GLFW_PRESS) && (mode != VIEW_MODE_POINTS)) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDisable(GL_LIGHTING);
                view_mode = VIEW_MODE_POINTS;
            }
            break;
        }
        case GLFW_KEY_4: {
            if((action == GLFW_PRESS) && (mode != VIEW_MODE_WEIGHTS)) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                view_mode = VIEW_MODE_WEIGHTS;
            }
            break;
        }
        case GLFW_KEY_5: {
            if((action == GLFW_PRESS) && (mode != VIEW_MODE_SKELETON)) {
                view_mode = VIEW_MODE_SKELETON;
            }
            break;
        }

        case GLFW_KEY_ESCAPE: {
            if(action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
            break;
        }
    }
}

/* Hogs of War directory structure...
 *     Audio
 *     Chars
 *     FEBmps
 *     FESounds
 *     FEText
 *     Maps
 *     Skys
 *     Speech
 */

int main(int argc, char **argv) {

    plInitialize(argc, argv, PL_SUBSYSTEM_LOG | PL_SUBSYSTEM_LOG);
    plClearLog(LOG);

    PRINT(" = = = = = = = = = = = = = = = = = = = = = = =\n");
    PRINT("   H0G Loader, created by Mark \"hogsy\" Sowden\n");
    PRINT(" = = = = = = = = = = = = = = = = = = = = = = =\n");
    //PRINT("Arguments:\n");
    //PRINT("    -gamedir <path>      required path to your Hogs of War directory (e.g. \"C:\\Hogs of War\").\n");
    PRINT("    -model <path>        opens up model for previewing (exclude file extension).\n");
    //PRINT("    -setup <path>        extracts content from packages.\n");
    PRINT("\n");

    plScanDirectory("./Chars/", ".MAD", extract_mad_file);
    plScanDirectory("./Chars/", ".MTD", extract_mad_file);
    plScanDirectory("./Maps/", ".MAD", extract_mad_file);
    plScanDirectory("./Maps/", ".MTD", extract_mad_file);

    const char *arg;
    if ((arg = plGetCommandLineArgument("-model")) && (arg[0] != '\0')) {
        memset(&model, 0, sizeof(PIGModel));

        if (!glfwInit()) {
            plMessageBox(TITLE, "Failed to initialize GLFW!\n");
            return -1;
        }

        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

        GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, arg, NULL, NULL);
        if (!window) {
            glfwTerminate();

            plMessageBox(TITLE, "Failed to create window!\n");
            return -1;
        }

        glfwSetKeyCallback(window, key_callback);
        glfwMakeContextCurrent(window);

        plInitialize(argc, argv, PL_SUBSYSTEM_GRAPHICS);

        char vtx_path[PL_SYSTEM_MAX_PATH] = {'\0'};
        snprintf(vtx_path, sizeof(vtx_path), "./%s.vtx", arg);
        char fac_path[PL_SYSTEM_MAX_PATH] = {'\0'};
        snprintf(fac_path, sizeof(fac_path), "./%s.fac", arg);
        char no2_path[PL_SYSTEM_MAX_PATH] = {'\0'};
        snprintf(no2_path, sizeof(no2_path), "./%s.no2", arg);

        load_hir_file("./Chars/pig.HIR");
        load_vtx_file(vtx_path);
        load_fac_file(fac_path);

        plSetDefaultGraphicsState();
        plSetClearColour(plCreateColour4b(0, 0, 128, 255));

        PLCamera *camera = plCreateCamera();
        if (!camera) {
            PRINT("Failed to create camera!");
            return -1;
        }
        camera->mode = PL_CAMERAMODE_PERSPECTIVE;
        camera->fov = 90.f;

        glfwGetFramebufferSize(window, (int *) &camera->viewport.width, (int *) &camera->viewport.height);

        plSetCameraPosition(camera, plCreateVector3D(0, 12, -500));

#if 1
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // ol' gl lighting, just for testing
        GLfloat light_ambient[] = {0.6f, 0.6f, 0.6f, 1.f};
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
        glEnable(GL_LIGHT1);
        GLfloat light_colour_red[] = {0.5f, 0, 0, 1.f};
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light_colour_red);
        GLfloat light_position[] = {0, 12.f, -800.f};
        glLightfv(GL_LIGHT1, GL_POSITION, light_position);

        glPointSize(5.f);
        glLineWidth(2.f);
#endif

        while (!glfwWindowShouldClose(window)) {

            glfwPollEvents();

            // input handlers start..
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            // Camera rotation
            static double oldmpos[2] = {0, 0};
            int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (state == GLFW_PRESS) {
                double nxpos = xpos - oldmpos[0];
                double nypos = ypos - oldmpos[1];
                model.angles.x += (nxpos / 100.f);
                model.angles.y += (nypos / 100.f);
            } else {
                oldmpos[0] = xpos;
                oldmpos[1] = ypos;
            }

            // Zoom in and out thing...
            static double oldrmpos[2] = {0, 0};
            state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (state == GLFW_PRESS) {
                double nypos = ypos - oldrmpos[1];
                camera->position.z += (nypos / 20.f);
            } else {
                oldrmpos[0] = xpos;
                oldrmpos[1] = ypos;
            }
            // input handlers end...

            plClearBuffers(PL_BUFFER_COLOUR | PL_BUFFER_DEPTH | PL_BUFFER_STENCIL);

            plSetupCamera(camera);

#if 1
            glLoadIdentity();
            glRotatef(model.angles.y, 1, 0, 0);
            glRotatef(model.angles.x, 0, 1, 0);
            glRotatef(model.angles.z + 180.f, 0, 0, 1);

            switch (view_mode) {
                default:
                    break;

                case VIEW_MODE_LIT: {
                    glEnable(GL_LIGHTING);
                    glShadeModel(GL_FLAT);

                    plDrawMesh(model.tri_mesh);

                    glShadeModel(GL_SMOOTH);
                    glDisable(GL_LIGHTING);
                    break;
                }

                case VIEW_MODE_POINTS: {
                    plDrawMesh(model.vertex_mesh);
                    break;
                }

                case VIEW_MODE_WEIGHTS:
                case VIEW_MODE_WIREFRAME: {
                    plDrawMesh(model.tri_mesh);
                    break;
                }

                case VIEW_MODE_SKELETON: {
                    plDrawMesh(model.tri_mesh);
                    glDisable(GL_DEPTH_TEST);
                    plDrawMesh(model.skeleton_mesh);
                    glEnable(GL_DEPTH_TEST);
                }
            }
#endif

            glfwSwapBuffers(window);
        }

        plDeleteMesh(model.tri_mesh);
        plDeleteMesh(model.vertex_mesh);
        plDeleteMesh(model.skeleton_mesh);

        plDeleteCamera(camera);

        glfwTerminate();
    }

    plShutdown();

    return 0;
}