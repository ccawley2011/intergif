/* LoadPNG.c */

/* PNG file code for InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Cameron Cawley
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "animlib.h"
#include "utils.h"

#ifdef ENABLE_PNG

#include <png.h>

#if 0
#define debugf printf
#define DEBUG 1
#else
#define debugf 1?0:printf
#define DEBUG 0
#endif


#define PNG_HAS_PALETTE(colour_type) (colour_type & PNG_COLOR_MASK_PALETTE)
#define PNG_IS_INDEXED(colour_type) \
	(PNG_HAS_PALETTE(colour_type) || colour_type == PNG_COLOR_TYPE_GRAY)

png_bytepp create_rowptr(png_voidp buffer, png_uint_32 bytewidth, png_uint_32 height) {
    png_uint_32 i;
    png_bytepp row_ptr = Anim_Allocate(height * sizeof(png_bytep));
    if (!row_ptr)
        return NULL;

    for (i = 0; i < height; i++)
         row_ptr[i] = (png_bytep)buffer + (bytewidth * i);

    return row_ptr;
}


typedef struct {
    png_const_bytep data;
    png_uint_32 nSize;
    png_uint_32 nPos;
} memory_io_state;

static void read_data_memory(png_structp png_ptr, png_bytep data, png_uint_32 nSize) {
    memory_io_state *f = png_get_io_ptr(png_ptr);

    if (nSize > (f->nSize - f->nPos))
        png_error(png_ptr, "Failed to read data from memory");

    memcpy(data, f->data + f->nPos, nSize);
    f->nPos += nSize;
}


    /*=================*
     *   Constructor   *
     *=================*/


BOOL Anim_ConvertPNG( const void *data, size_t nSize,
                      anim_animfn animfn, anim_imagefn fn, void *handle )
{
    int nFrames;
    pixel *buffer;
    BOOL result = FALSE;
    anim_imageinfo pixels;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_uint_32 width, height,  bytewidth;
    int bit_depth, colour_type, pixel_depth;
    memory_io_state fp;
    unsigned int pal[512];

    /* Check it's a PNG file, if not return FALSE */

    if ( png_sig_cmp(data, 0, 8) != 0 )
        return FALSE;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        Anim_NoMemory( "frompng" );
        return FALSE;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        Anim_NoMemory( "frompng" );
        png_destroy_read_struct (&png_ptr, NULL, NULL);
        return FALSE;
    }

    fp.data = data;
    fp.nSize = nSize;
    fp.nPos = 8;
    png_set_read_fn(png_ptr, &fp, (png_rw_ptr)read_data_memory);

    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                 &colour_type, NULL, NULL, NULL);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (!PNG_IS_INDEXED(colour_type))
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_set_packswap(png_ptr);
    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                 &colour_type, NULL, NULL, NULL);

    pixel_depth = bit_depth * png_get_channels(png_ptr, info_ptr);
    bytewidth = (png_get_rowbytes(png_ptr, info_ptr) + 3) & ~3;

    if ( animfn )
    {
        if ( !(*animfn)( handle, width, height, 0 ) )
        {
            debugf( "convertpng: animfn returned FALSE, exiting\n" );
            return FALSE;
        }
    }

    if (PNG_IS_INDEXED(colour_type)) {
        int i, palette_count = 0;
        png_colorp plte = NULL;

        if (colour_type == PNG_COLOR_TYPE_GRAY) {
            palette_count = 1 << bit_depth;
            plte = Anim_Allocate( palette_count * sizeof(png_color) );
            png_build_grayscale_palette(bit_depth, plte);
        } else {
            png_get_PLTE(png_ptr, info_ptr, &plte, &palette_count);
        }

        if (plte) {
		for (i = 0; i < palette_count; i++) {
			pal[i] = (plte[i].red << 8) | (plte[i].green << 16) | (plte[i].blue << 24);
		}
	}

        if (colour_type == PNG_COLOR_TYPE_GRAY)
            Anim_Free( &plte );
    }

    /* Buffer */

    buffer = Anim_Allocate( bytewidth*height );
    if ( !buffer )
    {
        Anim_NoMemory( "frompng" );
        return FALSE;
    }

    /* The frames */

    nFrames = 0;
    while ( nFrames == 0 )
    {
        /* Decompress the frame */

        debugf( "Decompressing frame %d\n", nFrames );

        png_bytepp row_ptr = create_rowptr(buffer, bytewidth, height);
        if (!row_ptr) {
            Anim_NoMemory( "frompng" );
            goto err;
        }
        png_read_image(png_ptr, row_ptr);
        png_read_end(png_ptr, NULL);
        Anim_Free( &row_ptr );

        pixels.nWidth = width;
        pixels.nLineWidthBytes = bytewidth;
        pixels.nHeight = height;
        pixels.nBPP = pixel_depth;
        pixels.pBits = buffer;
        pixels.csDelay = 8;

        if ( !(*fn)( handle, &pixels, -1, NULL,
                     PNG_IS_INDEXED(colour_type) ? pal : NULL ) )
        {
            debugf( "convertpng: fn returned FALSE, exiting\n" );
            goto err;
        }

        nFrames++;
    }

    result = TRUE;

err:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    Anim_Free( &buffer );
    return result;
}

#endif

/* eof */
