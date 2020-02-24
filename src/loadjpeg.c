/* LoadJPEG.c */

/* JPEG file code for InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#include "animlib.h"
#include "sprite.h"


#ifdef __acorn

#include <kernel.h>
#include <swis.h>

#ifndef JPEG_Info
#define JPEG_Info 0x49980
#endif
#ifndef JPEG_PlotScaled
#define JPEG_PlotScaled 0x49982
#endif

/*---------------------------------------------------------------------------*
 * Constructor                                                               *
 *---------------------------------------------------------------------------*/

BOOL Anim_ConvertJPEG( const void *data, size_t nSize,
                       anim_animfn animfn, anim_imagefn fn, void *handle )
{
    int i, r,g,b;
    _kernel_swi_regs regs,sos;
    int areasize;
    spritearea area = NULL;
    spritestr *pSprite;
    unsigned int palette[256];
    unsigned int *pPal;
    int w,h;
    anim_imageinfo pixels;
    _kernel_oserror *e = NULL;
    BOOL bits24 = TRUE;
    modeval spritemode;

    /* check it's a JPEG file, if not return NULL */

    if ( (*(int*)data & 0xFFFF) != 0xD8FF )
        return FALSE;

    regs.r[0] = 0;
    _kernel_swi( TaskWindow_TaskInfo, &regs, &regs );
    if ( regs.r[0] != 0 )
    {
        Anim_SetError( "InterGif cannot process JPEG files in a taskwindow. "
                     "Please either press F12 or use the desktop front-end." );
        return FALSE;
    }

    regs.r[0] = 1;
    regs.r[1] = (int)data;
    regs.r[2] = nSize;
    e =  _kernel_swi( JPEG_Info, &regs, &regs );
    if ( e )
    {
        Anim_SetError( "Invalid JPEG file: %s\n", e->errmess );
        return FALSE;
    }

    w = regs.r[2];
    h = regs.r[3];

    if ( animfn && !(*animfn)( handle, w, h, 0 ) )
    {
        debugf( "convertjpeg: animfn returned FALSE, exiting\n" );
        return FALSE;
    }

    debugf( "convertjpeg: w=%d h=%d\n", w, h );

tryagain:

    if ( bits24 )
    {
        areasize = h*w*4 + sizeof(spritestr);
        areasize += sizeof(spriteareastr);
    }
    else
    {
        areasize = h * ( (w+3) & ~3) + sizeof(spritestr);
        areasize += 256*8 + sizeof(spriteareastr);
    }

    debugf( "fromjpeg: Allocating %d bytes\n", areasize );

    area = Anim_Allocate( areasize );

    if ( !area )
    {
        Anim_NoMemory( "fromjpeg2" );
        return FALSE;
    }

    debugf( "fromjpeg: Allocate succeeded\n" );

    area->nSize = areasize;
    area->nSprites = 0;
    area->nFirstOffset =
        area->nFreeOffset = 16;

    debugf( "Creating sprite (%dx%d,%d bytes)\n",
            w, h, areasize );

    if (bits24) {
        spritemode.sprite_mode.isType = TRUE;
        spritemode.sprite_mode.horz_dpi = 90;
        spritemode.sprite_mode.vert_dpi = 90;
        spritemode.sprite_mode.type = 6;
        spritemode.sprite_mode.isWide = FALSE;
    } else {
        spritemode.screen_mode = 28;
    }

    regs.r[0] = 0x10F;
    regs.r[1] = (int)area;
    regs.r[2] = (int)"jpeg";
    regs.r[3] = 0;
    regs.r[4] = w;
    regs.r[5] = h;
    regs.r[6] = spritemode.screen_mode;
    e = _kernel_swi( OS_SpriteOp, &regs, &regs);

    /* OS_SpriteOp 15 will give an error on old (non-24-bit-capable) machines
     * so try again in 8bit
     */
    if ( e && bits24 )
    {
        Anim_Free( &area );
        bits24 = FALSE;
        goto tryagain;
    }

#if DEBUG
    if ( e )
    {
        debugf( "fromjpeg: spritecreate fails, %s\n", e->errmess );
    }
#endif
    debugf( "areasize=%d, freeoffset now %d\n", areasize, area->nFreeOffset );

    pSprite = (spritestr*)((char*)area + 16);

    if ( !bits24 )
    {
        debugf( "Building palette\n" );

        pPal = palette;
        memset( pPal, 0, 256*4 );
        pPal[255] = 0xFFFFFF00;
        for ( r=0; r<6; r++ )
            for ( g=0; g<6; g++ )
                for ( b=0; b<6; b++ )
                    *pPal++ = r*0x33000000 + g*0x330000 + b*0x3300;

        debugf( "Copying palette into sprite\n" );

        pPal = (unsigned int*)(pSprite+1);

        for ( i=0; i<255; i++ )
        {
            *pPal++ = palette[i];
            *pPal++ = palette[i];
        }

        pSprite->imageoffset += 256*8;
        pSprite->maskoffset  += 256*8;
        pSprite->nNextOffset += 256*8;
        area->nFreeOffset    += 256*8;
    }

    debugf( "areasize=%d, freeoffset now %d\n", areasize, area->nFreeOffset );

    regs.r[0] = -1;
    _kernel_swi(Wimp_CommandWindow, &regs, &regs);

    debugf( "Plotting jpeg file\n" );

    sos.r[0] = 0x13C;
    sos.r[1] = (int)area;
    sos.r[2] = (int)"jpeg";
    sos.r[3] = 0;
    e = _kernel_swi( OS_SpriteOp, &sos, &sos );
    if ( !e )
    {
        regs.r[0] = (int)data;
        regs.r[1] = 0;
        regs.r[2] = 0;
        regs.r[3] = 0;
        regs.r[4] = nSize;
        regs.r[5] = 3;
        _kernel_swi( JPEG_PlotScaled, &regs, &regs );
        _kernel_swi( OS_SpriteOp, &sos, &sos );
    }
    else
    {
        Anim_SetError( "fromjpeg: plot failed, %s", e->errmess );
        Anim_Free( &area );
        return FALSE;
    }

#if DEBUG
    {
        FILE *f = fopen( "igdebug", "wb" );
        fwrite( &area->nSprites, 1, area->nSize-4, f );
        fclose(f);
    }
#endif

    regs.r[0] = -1;
    _kernel_swi(Wimp_CommandWindow, &regs, &regs);

    /* Compress the frame */

    pixels.pBits = (char*)pSprite + pSprite->imageoffset;
    pixels.nWidth = w;
    pixels.nLineWidthBytes = bits24 ? w*4 : w;
    pixels.nHeight = h;
    pixels.nBPP = bits24 ? 32 : 8;
    pixels.csDelay = 8;

    if ( !(*fn)( handle, &pixels, bits24 ? -1 : 255, NULL,
                 bits24 ? NULL : palette ) )
    {
        debugf("convertjpeg: fn returned FALSE, exiting\n" );
        Anim_Free( &area );
        return FALSE;
    }

    Anim_Free( &area );

    return TRUE;
}
#endif

/* eof */
