/* LoadDraw.c */

/* RiscOS Draw file code for InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Peter Hartley       <pdh@chaos.org.uk>
 *
 * History:
 *      05-Feb-97 pdh Adapted from aadraw: c.tosprite
 *      07-Feb-97 *** Release 5.01
 *      10-Mar-97 pdh Frob a lot for new anim library
 *      07-Apr-97 *** Release 6beta1
 *      20-May-97 *** Release 6beta2
 *      24-Aug-97 *** Release 6
 *      27-Sep-97 *** Release 6.01
 *      08-Nov-97 *** Release 6.02
 *      10-Feb-98 pdh Anti-alias in strips if short of memory
 *      21-Feb-98 *** Release 6.03
 *      07-Jun-98 *** Release 6.04
 *      01-Aug-98 pdh Frob for anim_imagefn stuff
 *      21-Aug-98 *** Release 6.05
 *      05-Oct-98 pdh Disable Wimp_CommandWindow; fix error reporting
 *      05-Oct-98 *** Release 6.06
 *      19-Feb-99 *** Release 6.07
 *      26-Mar-00 *** Release 6.10
 *      10-Dec-00 pdh Fix 8bpp code
 *      10-Dec-00 *** Release 6.12
 *
 * References:
 *      http://utter.chaos.org.uk/~pdh/software/aadraw.htm
 *          pdh's AADraw home page
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#include "animlib.h"
#include "antialias.h"
#include "sprite.h"


typedef struct
{
    int x0,y0;
    int x1,y1;
    int x2,y2;
} draw_matrix;

typedef struct
{
    int x0,y0;
    int x1,y1;
} draw_box;

#ifdef __acorn

#include <kernel.h>
#include <swis.h>

/*---------------------------------------------------------------------------*
 * Constructor                                                               *
 *---------------------------------------------------------------------------*/

BOOL Anim_ConvertDraw( const void *data, size_t nSize,
                       anim_animfn animfn, anim_imagefn fn, void *handle )
{
    draw_matrix tm = { 0x10000 * ANIM_AAFACTOR, 0,
                       0, 0x10000 * ANIM_AAFACTOR,
                       0, 0 };
    draw_box box;
    int spritex, spritey;
    int i, r,g,b;
    _kernel_swi_regs regs,sos,draw;
    int areasize;
    spritearea area = NULL;
    spritestr *pSprite;
    unsigned int palette[256];
    unsigned int *pPal;
    int w,h;
    int pass, passes, sectiony, basey2;
    anim_imageinfo pixels;
    _kernel_oserror *e = NULL;
    BOOL bits24 = TRUE;
    modeval spritemode;

    /* check it's a Draw file, if not return NULL */

    if ( *(int*)data != 0x77617244 )            /* "Draw" */
        return FALSE;

    regs.r[0] = 0;
    _kernel_swi( TaskWindow_TaskInfo, &regs, &regs );
    if ( regs.r[0] != 0 )
    {
        Anim_SetError( "InterGif cannot process Draw files in a taskwindow. "
                     "Please either press F12 or use the desktop front-end." );
        return FALSE;
    }

    regs.r[0] = 18;
    regs.r[1] = (int)"DrawFile";
    if ( _kernel_swi( OS_Module, &regs, &regs ) )
    {
        regs.r[0] = 1;
        regs.r[1] = (int)"System:Modules.Drawfile";
        e = _kernel_swi( OS_Module, &regs, &regs );

        if ( e )
        {
            Anim_SetError( "DrawFile module not loaded: %s", e->errmess );
            return FALSE;
        }
    }

    draw.r[0] = 0;
    draw.r[1] = (int)data;
    draw.r[2] = nSize;
    draw.r[3] = (int)&tm;
    draw.r[4] = (int)&box;
    if ( _kernel_swi( DrawFile_BBox, &draw, &draw ) )
    {
        Anim_SetError( "Invalid Draw file\n" );
        return FALSE;
    }

    tm.x2 = 3584-box.x0;
    tm.y2 = basey2 = 3584-box.y0;
    box.x0 = box.x0 / 256;
    box.x1 = box.x1 / 256;
    box.y0 = box.y0 / 256;
    box.y1 = box.y1 / 256;

    w = (box.x1 - box.x0)/2 + 16;
    w = (w + ANIM_AAFACTOR - 1) / ANIM_AAFACTOR;

    h = (box.y1 - box.y0)/2 + 16;
    h = (h + ANIM_AAFACTOR - 1) / ANIM_AAFACTOR;

    if ( animfn && !(*animfn)( handle, w, h, 0 ) )
    {
        debugf( "convertdraw: animfn returned FALSE, exiting\n" );
        return FALSE;
    }

    debugf( "convertdraw: w=%d h=%d box=(%d,%d)..(%d,%d)\n", w, h,
        box.x0, box.y0, box.x1, box.y1 );

tryagain:

    if ( bits24 )
        pixels.pBits = Anim_Allocate( w*h*4 );
    else
        pixels.pBits = Anim_Allocate( w*h ); /* Note not word-aligned */

    if ( !pixels.pBits )
    {
        Anim_NoMemory( "fromdraw" );
        return FALSE;
    }

#if DEBUG
    memset( pixels.pBits, 91, w*h );
#endif

    spritex = w * ANIM_AAFACTOR;
    spritey = h * ANIM_AAFACTOR;

    /* Plotting it three times the size and anti-aliasing down produces very
     * nice results, but needs lllots of memory. If memory's a bit short, we
     * try doing it in strips in a multi-pass fashion -- not unlike !Printers.
     */

    for ( passes = 1; passes < 65; passes*=2 )
    {
        sectiony = (( h + passes-1 ) / passes) * ANIM_AAFACTOR;

        if ( bits24 )
        {
            areasize = sectiony*spritex*4 + sizeof(spritestr);
            areasize += sizeof(spriteareastr);
        }
        else
        {
            areasize = sectiony * ( (spritex+3) & ~3) + sizeof(spritestr);
            areasize += 256*8 + sizeof(spriteareastr);
        }

        if ( !sectiony )
            break;

        debugf( "fromdraw: Allocating %d bytes\n", areasize );

        area = Anim_Allocate( areasize );

        if ( area )
            break;

        debugf( "fromdraw: Allocate failed, trying smaller\n" );
    }

    if ( !area )
    {
        Anim_NoMemory( "fromdraw2" );
        Anim_Free( &pixels.pBits );
        return FALSE;
    }

    debugf( "fromdraw: Allocate succeeded\n" );

    area->nSize = areasize;
    area->nSprites = 0;
    area->nFirstOffset =
        area->nFreeOffset = 16;

    debugf( "Creating big sprite (%dx%d,%d bytes) total size would be %dx%d\n",
            spritex, sectiony, areasize, spritex, spritey );

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
    regs.r[2] = (int)"drawfile";
    regs.r[3] = 0;
    regs.r[4] = spritex;
    regs.r[5] = sectiony;
    regs.r[6] = spritemode.screen_mode;
    e = _kernel_swi( OS_SpriteOp, &regs, &regs);

    /* OS_SpriteOp 15 will give an error on old (non-24-bit-capable) machines
     * so try again in 8bit
     */
    if ( e && bits24 )
    {
        Anim_Free( &pixels.pBits );
        Anim_Free( &area );
        bits24 = FALSE;
        goto tryagain;
    }

#if DEBUG
    if ( e )
    {
        debugf( "fromdraw: spritecreate fails, %s\n", e->errmess );
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

    for ( pass=0; pass<passes; pass++ )
    {
        int thisy = sectiony/ANIM_AAFACTOR;
        int offset = (pass*sectiony)/ANIM_AAFACTOR;

        tm.y2 = basey2 - (h-offset-thisy)*256*ANIM_AAFACTOR*2;

        if ( thisy + offset > h )
            thisy = h - offset;

        debugf( "Pass %d: thisy=%d offset=%d\n", pass, thisy, offset );

        offset *= w;

        debugf( "Blanking sprite\n" );

        memset( ((char*)pSprite) + pSprite->imageoffset, 255,
                (pSprite->width+1)*sectiony*4 );

        debugf( "Plotting drawfile\n" );

        sos.r[0] = 0x13C;
        sos.r[1] = (int)area;
        sos.r[2] = (int)"drawfile";
        sos.r[3] = 0;
        e = _kernel_swi( OS_SpriteOp, &sos, &sos );
        if ( !e )
        {
            draw.r[4] = 0;
            _kernel_swi( DrawFile_Render, &draw, &draw );
            _kernel_swi( OS_SpriteOp, &sos, &sos );
        }

#if DEBUG
        {
            FILE *f = fopen( "igdebug", "wb" );
            fwrite( &area->nSprites, 1, area->nSize-4, f );
            fclose(f);
        }
#endif

        if ( e )
        {
            debugf( "Error: %s\n", e->errmess );
            break;
        }

        debugf( "Anti-aliasing\n" );

        if ( bits24 )
        {
            Anim_AntiAlias24( (unsigned int*)( (char*)pSprite + pSprite->imageoffset ),
                              spritex,
                              ((unsigned int*)pixels.pBits) + offset,
                              w, thisy );
        }
        else
        {
            Anim_AntiAlias( (pixel*)pSprite + pSprite->imageoffset,
                            (spritex+3) & ~3,
                            ((pixel*)pixels.pBits) + offset,
                            w, thisy );
        }
    }

    Anim_Free( &area );

    if ( e )
    {
        Anim_SetError( "fromdraw: plot failed, %s", e->errmess );
        Anim_Free( &pixels.pBits );
        return FALSE;
    }

    regs.r[0] = -1;
    _kernel_swi(Wimp_CommandWindow, &regs, &regs);

    /* Compress the frame */

    pixels.nWidth = w;
    pixels.nLineWidthBytes = bits24 ? w*4 : w;
    pixels.nHeight = h;
    pixels.nBPP = bits24 ? 32 : 8;
    pixels.csDelay = 8;

#if DEBUG
    {
        spriteareastr sai;
        int imgsize = h*pixels.nLineWidthBytes;
        spritestr sh;
        FILE *f = fopen("igdebug2", "wb");

        sai.nSprites = 1;
        sai.nFirstOffset = 16;
        sai.nFreeOffset = 16 + sizeof(spritestr) + imgsize;
        fwrite( &sai.numsprites, 1, 12, f );
        sh.nNextOffset = sizeof(spritestr) + imgsize;
        strncpy( sh.name, "drawfile2", 12 );
        sh.width = (pixels.nLineWidthBytes/4)-1;
        sh.height = h-1;
        sh.leftbit = 0;
        sh.rightbit = 31;
        sh.imageoffset = sh.maskoffset = sizeof(spritestr);
        if (bits24) {
            spritemode.sprite_mode.isType = TRUE;
            spritemode.sprite_mode.horz_dpi = 90;
            spritemode.sprite_mode.vert_dpi = 90;
            spritemode.sprite_mode.type = 6;
            spritemode.sprite_mode.isWide = FALSE;
        } else {
            spritemode.screen_mode = 28;
        }
        fwrite( &sh, 1, sizeof(spritestr), f );
        fwrite( pixels.pBits, 1, imgsize, f );
        fclose( f );
    }
#endif

    if ( !(*fn)( handle, &pixels, bits24 ? -1 : 255, NULL,
                 bits24 ? NULL : palette ) )
    {
        debugf("convertdraw: fn returned FALSE, exiting\n" );
        Anim_Free( &pixels.pBits );
        return FALSE;
    }

    Anim_Free( &pixels.pBits );

    return TRUE;
}
#endif


/*---------------------------------------------------------------------------*
 * Anti-aliasing routine -- reduces by a scale of ANIM_AAFACTOR in each      *
 * direction.                                                                *
 * Does sensible things with background areas so it's easy to make a true    *
 * transparent GIF from the output.                                          *
 * Using a regular colour cube to dither into makes the code trivial         *
 * (compared to what ChangeFSI must do, for instance) -- but that's still no *
 * excuse for not doing Floyd-Steinberg :-(                                  *
 *---------------------------------------------------------------------------*/

void Anim_AntiAlias( const pixel *srcBits, unsigned int abwSrc,
                     pixel *destBits, unsigned int w, unsigned int h )
{
    unsigned int r,g,b,t,x,y,i,j;
    char byte;
    char bgbyte;
    int abwDest = w; /* Dest is byte-packed, NOT sprite format */
    char ra[256],ga[256],ba[256];
    char dividebyf2[ANIM_AAFACTOR*ANIM_AAFACTOR*6];

    /* Precalculate divisions by 6 to speed up dither */
    for (r=0; r<6; r++)
        for (g=0; g<6; g++)
            for (b=0; b<6; b++)
            {
                ra[r*36+g*6+b] = r;
                ga[r*36+g*6+b] = g;
                ba[r*36+g*6+b] = b;
            }

    /* Precalculate divisions by ANIM_AAFACTOR*ANIM_AAFACTOR */
    for ( i=0; i < ANIM_AAFACTOR*ANIM_AAFACTOR*6; i++ )
    {
        j = (i<<8) / (ANIM_AAFACTOR*ANIM_AAFACTOR);
        dividebyf2[i] = (j>>8); /* + ((j&128)?1:0);*/
    }

    bgbyte = 215;

    for ( y=0; y < h; y++ )
    {
        for ( x=0; x < w; x++ )
        {
            r=g=b=t=0;
            for (i=0; i<ANIM_AAFACTOR; i++)
                for (j=0; j<ANIM_AAFACTOR; j++)
                {
                    byte = srcBits[x*ANIM_AAFACTOR+i+j*abwSrc];
                    if ( byte==255 )
                    {
                        byte=bgbyte;
                        t++;
                    }
                    b += ba[(int)byte]; g += ga[(int)byte]; r += ra[(int)byte];
                }
            if ( t == ANIM_AAFACTOR*ANIM_AAFACTOR )
                destBits[x] = 255;
            else
            {
                destBits[x] = 36 * dividebyf2[r]
                            +  6 * dividebyf2[g]
                            +      dividebyf2[b];
            }
        }
        destBits += abwDest;
        srcBits += abwSrc*ANIM_AAFACTOR;
    }
}

void Anim_AntiAlias24( const unsigned int *srcBits, unsigned int wSrc,
                       unsigned int *destBits, unsigned int w, unsigned int h )
{
    unsigned int r,g,b,t,x,y,i,j,word;
    char dividebyf2[ANIM_AAFACTOR*ANIM_AAFACTOR*256];

    /* Precalculate divisions by ANIM_AAFACTOR^2 */
    for ( i=0; i < sizeof(dividebyf2); i++ )
    {
        j = (i<<8) / (ANIM_AAFACTOR*ANIM_AAFACTOR);
        dividebyf2[i] = j>>8;
    }

    for ( y=0; y<h; y++ )
    {
        for ( x=0; x<w; x++ )
        {
            r=g=b=t=0;
            for ( i=0; i<ANIM_AAFACTOR; i++ )
                for ( j=0; j<ANIM_AAFACTOR; j++ )
                {
                    word = srcBits[x*ANIM_AAFACTOR+i+j*wSrc];
                    if ( word == (unsigned)-1 )
                    {
                        t++;
                    }
                    r += (word & 0xFF);
                    g += (word & 0xFF00)>>8;
                    b += (word & 0xFF0000)>>16;
                    /*b += (word & 0xFF000000)>>24;*/
                }

            if ( t == ANIM_AAFACTOR*ANIM_AAFACTOR )
                destBits[x] = (unsigned)-1;
            else
            {
                destBits[x] = ( dividebyf2[b] << 16 )
                            + ( dividebyf2[g] << 8 )
                            + ( dividebyf2[r] << 0 );
            }
        }
        destBits += w;
        srcBits += wSrc*ANIM_AAFACTOR;
    }
}

/* eof */
