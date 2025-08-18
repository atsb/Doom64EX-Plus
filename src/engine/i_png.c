// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    PNG image system
//
//-----------------------------------------------------------------------------

#include <math.h>
#include <png.h>

#include "i_png.h"
#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_swap.h"
#include "m_fixed.h"
#include "z_zone.h"
#include "w_wad.h"
#include "gl_texture.h"
#include "con_cvar.h"

static byte* pngWriteData;
static byte* pngReadData;
static size_t pngWritePos = 0;

CVAR_CMD(i_gamma, 0) {
    GL_DumpTextures();
}

//
// I_PNGRowSize
//
SDL_INLINE static size_t I_PNGRowSize(int width, byte bits) {
    if (bits >= 8) {
        return ((width * bits) >> 3);
    }
    else {
        return (((width * bits) + 7) >> 3);
    }
}

//
// I_PNGReadFunc
//
static void I_PNGReadFunc(png_structp ctx, byte* area, size_t size) {
    dmemcpy(area, pngReadData, size);
    pngReadData += size;
}

//
// I_PNGFindChunk
//
static int I_PNGFindChunk(png_struct* png_ptr, png_unknown_chunkp chunk) {
    int* dat;

    if (!dstrncmp((char*)chunk->name, "grAb", 4) && chunk->size >= 8) {
        dat = (int*)png_get_user_chunk_ptr(png_ptr);
        dat[0] = I_SwapBE32(*(int*)(chunk->data));
        dat[1] = I_SwapBE32(*(int*)(chunk->data + 4));
        return 1;
    }

    return 0;
}

//
// I_GetRGBGamma
//
SDL_INLINE static byte I_GetRGBGamma(int c) {
    return (byte)MIN(pow((float)c, (1.0f + (0.01f * i_gamma.value))), 255);
}

//
// I_TranslatePalette
// Increases the palette RGB based on gamma settings
//
static void I_TranslatePalette(png_colorp dest) {
    int i = 0;
    for (i = 0; i < 256; i++) {
        dest[i].red = I_GetRGBGamma(dest[i].red);
        dest[i].green = I_GetRGBGamma(dest[i].green);
        dest[i].blue = I_GetRGBGamma(dest[i].blue);
    }
}

//
// I_PNGReadData
//
byte* I_PNGReadData(int lump, bool palette, bool nopack, bool alpha,
    int* w, int* h, int* offset, int palindex)
{
    png_structp png_ptr = NULL;
    png_infop   info_ptr = NULL;
    png_uint_32 width, height;
    int         bit_depth, color_type, interlace_type;

    byte* png;
    byte* out;
    size_t row;
    png_bytep* row_pointers = NULL;

    // get lump data
    png = W_CacheLumpNum(lump, PU_STATIC);
    pngReadData = png;

    // setup struct
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == NULL) {
        I_Error("I_PNGReadData: Failed to read struct");
        return NULL;
    }

    // setup info struct
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        I_Error("I_PNGReadData: Failed to create info struct");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        I_Error("I_PNGReadData: Failed on setjmp");
        return NULL;
    }

    // setup callback function for reading data
    png_set_read_fn(png_ptr, NULL, I_PNGReadFunc);

    // look for offset chunk if specified
    if (offset) {
        offset[0] = 0;
        offset[1] = 0;
        png_set_read_user_chunk_fn(png_ptr, offset, I_PNGFindChunk);
    }

    // read png information
    png_read_info(png_ptr, info_ptr);

    // get IHDR chunk
    png_get_IHDR(
        png_ptr, info_ptr,
        &width, &height,
        &bit_depth, &color_type,
        &interlace_type, NULL, NULL);

    if (usingGL && !alpha) {
        int num_trans = 0;
        png_get_tRNS(png_ptr, info_ptr, NULL, &num_trans, NULL);
        if (num_trans || (color_type & PNG_COLOR_MASK_ALPHA)) {
            I_Error("I_PNGReadData: RGB8 PNG image (%s) has transparency", lumpinfo[lump].name);
        }
    }

    // indexed palette data!!!
    if (palette) {
        if (bit_depth == 4 && nopack) {
            png_set_packing(png_ptr);
        }
    }
    else {
        if (bit_depth == 16) {
            png_set_strip_16(png_ptr);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png_ptr);
        }

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            int i = 0, num_pal = 0;
            png_colorp pal = NULL;
            png_get_PLTE(png_ptr, info_ptr, &pal, &num_pal);

            if (palindex) {
                if (bit_depth == 4) {
                    for (i = 0; i < 16 && (16 * palindex + i) < num_pal; i++) {
                        dmemcpy(&pal[i], &pal[(16 * palindex) + i], sizeof(png_color));
                    }
                }
                else if (bit_depth >= 8) {
                    png_colorp pallump;
                    char palname[9];

                    sprintf(palname, "PAL");
                    dstrncpy(palname + 3, lumpinfo[lump].name, 4);
                    sprintf(palname + 7, "%i", palindex);

                    if (W_CheckNumForName(palname) != -1) {
                        pallump = W_CacheLumpName(palname, PU_STATIC);

                        for (i = 0; i < MIN(num_pal, 256); i++) {
                            pal[i].red = pallump[i].red;
                            pal[i].green = pallump[i].green;
                            pal[i].blue = pallump[i].blue;
                        }

                        Z_Free(pallump);
                    }
                    else {
                        for (i = 0; i < 16 && (16 * palindex + i) < num_pal; i++) {
                            dmemcpy(&pal[i], &pal[(16 * palindex) + i], sizeof(png_color));
                        }
                    }
                }
            }

            if (pal) {
                I_TranslatePalette(pal);
            }

            png_set_palette_to_rgb(png_ptr);
            color_type = PNG_COLOR_TYPE_RGB;
        }

        // Respect tRNS
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr);
        }

        if (alpha) {
            if (!(color_type & PNG_COLOR_MASK_ALPHA) && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
            }
            else {
            }
        }
        else {
            if ((color_type & PNG_COLOR_MASK_ALPHA) || png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                png_set_strip_alpha(png_ptr);
            }
        }
    }

    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(
        png_ptr, info_ptr,
        &width, &height,
        &bit_depth, &color_type,
        &interlace_type, NULL, NULL);

    if (w) *w = (int)width;
    if (h) *h = (int)height;

    const png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    out = (byte*)Z_Calloc(rowbytes * height, PU_STATIC, 0);
    row_pointers = (png_bytep*)Z_Malloc(sizeof(png_bytep) * height, PU_STATIC, 0);

    for (row = 0; row < height; row++) {
        row_pointers[row] = out + (row * rowbytes);
    }

    // read image
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, info_ptr);

    if (!palette && alpha) {
        const size_t expected = (size_t)width * 4;
        if (rowbytes == expected) {
            const size_t pixels = (size_t)width * (size_t)height;
            for (size_t p = 0; p < pixels; ++p) {
                byte r = out[4 * p + 0];
                byte g = out[4 * p + 1];
                byte b = out[4 * p + 2];
                byte a = out[4 * p + 3];

                out[4 * p + 0] = a; // A
                out[4 * p + 1] = b; // B
                out[4 * p + 2] = g; // G
                out[4 * p + 3] = r; // R
            }
        }
        else {
            size_t y;
            for (y = 0; y < height; ++y) {
                byte* rowptr = out + y * rowbytes;
                for (size_t x = 0; x < width; ++x) {
                    byte r = rowptr[4 * x + 0];
                    byte g = rowptr[4 * x + 1];
                    byte b = rowptr[4 * x + 2];
                    byte a = rowptr[4 * x + 3];

                    rowptr[4 * x + 0] = a;
                    rowptr[4 * x + 1] = b;
                    rowptr[4 * x + 2] = g;
                    rowptr[4 * x + 3] = r;
                }
            }
        }
    }

    // cleanup
    Z_Free(row_pointers);
    Z_Free(png);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return out;
}

//
// I_PNGWriteFunc
//
static void I_PNGWriteFunc(png_structp png_ptr, byte* data, size_t length) {
    pngWriteData = (byte*)Z_Realloc(pngWriteData, pngWritePos + length, PU_STATIC, 0);
    dmemcpy(pngWriteData + pngWritePos, data, length);
    pngWritePos += length;
}

//
// I_PNGCreate
//
byte* I_PNGCreate(int width, int height, byte* data, int* size) {
    png_structp png_ptr;
    png_infop   info_ptr;
    int         i = 0;
    byte* out;
    byte* pic;
    byte** row_pointers;
    size_t      j = 0;
    size_t      row;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == NULL) {
        I_Error("I_PNGCreate: Failed getting png_ptr");
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        I_Error("I_PNGCreate: Failed getting info_ptr");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        I_Error("I_PNGCreate: Failed on setjmp");
        return NULL;
    }

    png_set_write_fn(png_ptr, NULL, I_PNGWriteFunc, NULL);
    png_set_IHDR(
        png_ptr, info_ptr,
        width, height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    row_pointers = (byte**)Z_Malloc(sizeof(byte*) * height, PU_STATIC, 0);
    row = I_PNGRowSize(width, 24);
    pic = data;

    for (i = 0; i < height; i++) {
        row_pointers[i] = NULL;
        row_pointers[i] = (byte*)Z_Malloc(row, PU_STATIC, 0);

        for (j = 0; j < row; j++) {
            row_pointers[i][j] = *pic;
            pic++;
        }
    }

    Z_Free(data);

    png_write_image(png_ptr, row_pointers);

    // cleanup
    png_write_end(png_ptr, info_ptr);

    for (i = 0; i < height; i++) {
        Z_Free(row_pointers[i]);
    }

    Z_Free(row_pointers);
    row_pointers = NULL;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    out = (byte*)Z_Malloc(pngWritePos, PU_STATIC, 0);
    dmemcpy(out, pngWriteData, pngWritePos);
    *size = (int)pngWritePos;

    Z_Free(pngWriteData);
    pngWriteData = NULL;
    pngWritePos = 0;

    return out;
}
