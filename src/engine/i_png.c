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
#include <stdlib.h>
#include <string.h>

#include "i_png.h"
#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_swap.h"
#include "z_zone.h"
#include "w_wad.h"
#include "gl_texture.h"
#include "con_cvar.h"

// STB API for downscaling images and compressing them (so we load faster)

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define STBI_ONLY_PNG
#define STBI_NO_STDIO

#define STBI_WRITE_NO_STDIO
#define STBIW_PNG_COMPRESSION_LEVEL 1
#define STBIW_PNG_FILTER 0

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"

int PNG_DownscaleToFit(unsigned char* in_png, int in_size,
    int max_w, int max_h,
    unsigned char** out_png, int* out_size)
{
    *out_png = NULL; *out_size = 0;
    if (!in_png || in_size <= 0 || max_w <= 0 || max_h <= 0) 
        return 0;

    int w = 0, h = 0, comp = 0;
    unsigned char* rgba = stbi_load_from_memory(in_png, in_size, &w, &h, &comp, 4);
    if (!rgba)
        return 0;

    double sx = (double)max_w / (double)w;
    double sy = (double)max_h / (double)h;
    double s = sx < sy ? sx : sy;

    if (s >= 1.0) {
        unsigned char* out = (unsigned char*)malloc((size_t)in_size);
        if (!out) { 
            stbi_image_free(rgba); 
            return 0; 
        }
        memcpy(out, in_png, (size_t)in_size);
        *out_png = out; *out_size = in_size;
        stbi_image_free(rgba);
        return 1;
    }

    int nw = (int)(w * s + 0.5); if (nw < 1) nw = 1;
    int nh = (int)(h * s + 0.5); if (nh < 1) nh = 1;

    unsigned char* out_rgba = (unsigned char*)malloc((size_t)nw * nh * 4);
    if (!out_rgba) { 
        stbi_image_free(rgba); 
        return 0; 
    }

    unsigned char *okr = stbir_resize_uint8_linear(rgba, w, h, 0, out_rgba, nw, nh, 0, STBIR_RGBA);
    stbi_image_free(rgba);
    if (!okr) { 
        free(out_rgba); 
        return 0; 
    }

    size_t mem_size = 0;
    unsigned char* mem = stbi_write_png_to_mem(out_rgba, 0, nw, nh, 4, &mem_size);
    free(out_rgba);
    if (!mem || mem_size <= 0) 
        return 0;

    unsigned char* out = (unsigned char*)malloc(mem_size);
    if (!out) { 
        STBIW_FREE(mem); 
        return 0; 
    }
    memcpy(out, mem, mem_size);
    STBIW_FREE(mem);

    *out_png = out;
    *out_size = (int)mem_size;
    return 1;
}

int PNG_ReadDimensions(unsigned char* data, size_t size, int* out_w, int* out_h)
{
    if (!data || size < 24) 
        return 0;

    static const unsigned char sig[8] = { 
        137,80,78,71,13,10,26,10 
    };

    if (SDL_memcmp(data, sig, 8) != 0) 
        return 0; // not a PNG file

    if (size < 24) 
        return 0;

    if (SDL_memcmp(data + 12, "IHDR", 4) != 0) 
        return 0;

    unsigned w = (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19];
    unsigned h = (data[20] << 24) | (data[21] << 16) | (data[22] << 8) | data[23];
    if (w == 0 || h == 0) 
        return 0;
    *out_w = (int)w; *out_h = (int)h;
    return 1;
}

static byte* pngWriteData;
static byte* pngReadData;
static size_t   pngWritePos = 0;

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

static void I_TranslatePalette(png_colorp dest, int num_pal) {
    int i = 0;

    for (i = 0; i < num_pal; i++) {
        dest[i].red = I_GetRGBGamma(dest[i].red);
        dest[i].green = I_GetRGBGamma(dest[i].green);
        dest[i].blue = I_GetRGBGamma(dest[i].blue);
    }
}

static int multiply_alpha(int alpha, int color) {
    int  temp = (alpha * color) + 0x80;
    return (temp + (temp >> 8)) >> 8;
}

// adapted from png_do_strip_channel()
// https://github.com/pnggroup/libpng/blob/27de46c5a418d0cd8b2bded5a4430ff48deb2920/pngtrans.c#L494
static void rgba_to_premult_rgb_transform(png_structp png, png_row_infop  row_info, png_bytep row)
{
	png_bytep sp = row; /* source pointer */
	png_bytep dp = row; /* destination pointer */
	png_bytep ep = row + row_info->rowbytes; /* One beyond end of row */

	if (row_info->channels == 4 && row_info->bit_depth == 8) {

		/* Note that the loop adds 3 to dp and 4 to sp each time. */
		while (sp < ep)
		{
			int alpha = sp[3];
			*dp++ = multiply_alpha(alpha, *sp++);
			*dp++ = multiply_alpha(alpha, *sp++);
			*dp++ = multiply_alpha(alpha, *sp);
			sp += 2;
		}

		row_info->pixel_depth = 24;
		row_info->channels = 3;

		/* Finally fix the color type if it records an alpha channel */
		if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
			row_info->color_type = PNG_COLOR_TYPE_RGB;

		/* Fix the rowbytes value. */
		row_info->rowbytes = (size_t)(dp - row);
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
    png_uint_32 width = 0, height = 0;
    int bit_depth = 0, color_type = 0, interlace_type = 0;
    byte* src = NULL;
    byte* out = NULL;
    png_bytep* row_pointers = NULL;

    src = (byte*)W_CacheLumpNum(lump, PU_STATIC);
    pngReadData = src;

    if (W_LumpLength(lump) < 8 ||
        pngReadData[0] != (byte)0x89 || pngReadData[1] != 'P' || pngReadData[2] != 'N' || pngReadData[3] != 'G' ||
        pngReadData[4] != (byte)0x0D || pngReadData[5] != (byte)0x0A ||
        pngReadData[6] != (byte)0x1A || pngReadData[7] != (byte)0x0A)
    {
        Z_Free(src);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        Z_Free(src);
        I_Error("I_PNGReadData: Failed to create read struct");
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        Z_Free(src);
        I_Error("I_PNGReadData: Failed to create info struct");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        Z_Free(src);
        I_Error("I_PNGReadData: libpng error");
        return NULL;
    }

    png_set_crc_action(png_ptr, PNG_CRC_NO_CHANGE, PNG_CRC_QUIET_USE);

    png_set_read_fn(png_ptr, NULL, I_PNGReadFunc);

    if (offset) {
        offset[0] = 0; offset[1] = 0;
        png_set_read_user_chunk_fn(png_ptr, offset, I_PNGFindChunk);
    }

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
        &bit_depth, &color_type, &interlace_type, NULL, NULL);

    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        Z_Free(src);
        return NULL;
    }

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

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_colorp pal = NULL;
            int num_pal = 0;
            png_get_PLTE(png_ptr, info_ptr, &pal, &num_pal);

            if (pal && palindex) {
                int i;
                if (bit_depth == 4) {
                    png_colorp src_pal = pal;
                    int src_num_pal = num_pal;
                    char palname[9];
                    int pallump;

                    SDL_snprintf(palname, sizeof(palname), "P%s", lumpinfo[lump].name);
                    pallump = W_CheckNumForName(palname);

                    if(pallump != -1) {
                        // load palette separately as lump stored in doom64ex-plus.wad if it exists 
                        // for compatibility with libpng > 1.5.15 not supporting PNG with num_pal > 16 for bit_depth == 4
                        src_pal = W_CacheLumpName(palname, PU_STATIC);
                        src_num_pal = lumpinfo[pallump].size / sizeof(png_color);
                    }

                    for (i = 0; i < 16 && (16 * palindex + i) < src_num_pal; i++) {
                        dmemcpy(&pal[i], &src_pal[(16 * palindex) + i], sizeof(png_color));
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
                        for (i = 0; i < num_pal && i < 256; i++) {
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

            if (pal) I_TranslatePalette(pal, num_pal);
            png_set_palette_to_rgb(png_ptr);
            color_type = PNG_COLOR_TYPE_RGB;
        }

		if (alpha) {
			if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
				png_set_gray_to_rgb(png_ptr);
			}

			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				png_set_tRNS_to_alpha(png_ptr);
			}

			if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
				png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
			}
		}
		else {

            boolean alpha_handled = false;

            if (usingGL) { // !alpha is true
                int num_trans = 0;
                png_get_tRNS(png_ptr, info_ptr, NULL, &num_trans, NULL);
                if (num_trans || (color_type & PNG_COLOR_MASK_ALPHA)) {

                    /* convert RGBA to premultiplied alpha RGB. At least found in:

                       - BETA64 remastered WAD
                       "EVIL" lump (fading graphics at the end of the title map) that is PNG_COLOR_TYPE_PALETTE
                        with a single color (black) for transparent (num_trans = 1)
                       - Ethereal WAD
                       Credit graphics displayed before each map are PNG_COLOR_TYPE_PALETTE with 256 num_trans

                    */

                    png_set_user_transform_info(png_ptr, NULL, 8, 3);
                    png_set_read_user_transform_fn(png_ptr, rgba_to_premult_rgb_transform);
                    alpha_handled = true;
                }
            }

            if(!alpha_handled) {

                if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
                    png_set_gray_to_rgb(png_ptr);
                }
                if (color_type & PNG_COLOR_MASK_ALPHA) {
                    png_set_strip_alpha(png_ptr);
                }
            }
		}

    }

    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
        &bit_depth, &color_type, &interlace_type, NULL, NULL);

    png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    if (rowbytes == 0 || height > (SIZE_MAX / rowbytes)) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        Z_Free(src);
        return NULL;
    }

    /* ALWAYS allocate!!! */
    out = (byte*)Z_Malloc((size_t)rowbytes * (size_t)height, PU_STATIC, 0);

    row_pointers = (png_bytep*)Z_Calloc((size_t)height * sizeof(png_bytep), PU_STATIC, 0);
    for (png_uint_32 y = 0; y < height; ++y) {
        row_pointers[y] = out + y * rowbytes;
    }

    /* Decode */
    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, info_ptr);

    if (!palette && alpha) {
        int channels = png_get_channels(png_ptr, info_ptr);
        if (channels == 4) {
            for (png_uint_32 y = 0; y < height; ++y) {
                byte* p = out + y * rowbytes;
                for (png_uint_32 x = 0; x < width; ++x) {
                    byte a = p[3];
                    if (a == 0) {
                        p[0] = 0; p[1] = 0; p[2] = 0;
                    }
                    p += 4;
                }
            }
        }
    }

    if (w) 
        *w = (int)width;
    if (h) 
        *h = (int)height;

    Z_Free(row_pointers);
    Z_Free(src);
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
        png_ptr,
        info_ptr,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    row_pointers = (byte**)Z_Malloc(sizeof(byte*) * height, PU_STATIC, 0);
    row = I_PNGRowSize(width, 24 /*info_ptr->pixel_depth*/);
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
    png_write_end(png_ptr, info_ptr);

    for (i = 0; i < height; i++) {
        Z_Free(row_pointers[i]);
    }

    Z_Free(row_pointers);
    row_pointers = NULL;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    out = (byte*)Z_Malloc(pngWritePos, PU_STATIC, 0);
    dmemcpy(out, pngWriteData, pngWritePos);
    *size = pngWritePos;

    Z_Free(pngWriteData);
    pngWriteData = NULL;
    pngWritePos = 0;

    return out;
}
