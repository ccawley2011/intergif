/* LoadTCA.c */

/* The Complete Animator file code for InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Peter Hartley       <pdh@chaos.org.uk>
 *
 * History:
 *      23-Aug-96 pdh Created
 *      25-Aug-96 pdh Frob to cope with delta films
 *      25-Aug-96 *** Release 3.00
 *      01-Sep-96 pdh Added support for bLoop, nBackground fields
 *      01-Sep-96 *** Release 3.01
 *      27-Oct-96 *** Release 4beta1
 *      29-Oct-96 *** Release 4beta2
 *      07-Nov-96 *** Release 4
 *      15-Dec-96 *** Release 5beta1
 *      12-Jan-97 pdh Added support for pauses on individual frames
 *      27-Jan-97 *** Release 5beta2
 *      29-Jan-97 pdh Fix nasty memory bug in Decompress
 *      29-Jan-97 *** Release 5beta3
 *      03-Feb-97 *** Release 5
 *      07-Feb-97 *** Release 5.01
 *      11-Mar-97 pdh Frob for new anim library
 *      07-Apr-97 *** Release 6beta1
 *      20-May-97 *** Release 6beta2
 *      24-Aug-97 *** Release 6
 *      27-Sep-97 *** Release 6.01
 *      08-Nov-97 *** Release 6.02
 *      15-Feb-98 pdh Add palette mapper support
 *      21-Feb-98 *** Release 6.03
 *      07-Jun-98 *** Release 6.04
 *      01-Aug-98 pdh anim_imagefn stuff
 *      21-Aug-98 *** Release 6.05
 *      19-Feb-99 *** Release 6.07
 *      26-Mar-00 *** Release 6.10
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "animlib.h"
#include "utils.h"
#include "riemann.h"
#include "workspace.h"
#include "tca.h"


static const void *TCA_FindChunk( const void *data, unsigned int size, int id )
{
    tca_chunkpointer ptr;
    tca_chunkpointer end;

    ptr.pBytes = data;
    end.pBytes = ptr.pBytes + size;

    while ( ptr.pBytes < end.pBytes )
    {
        if ( ptr.pChunk->chunkid == id )
            return (void*)(ptr.pChunk+1);

        if ( ptr.pChunk->nSize <= sizeof(tca_chunk)
	     || ptr.pChunk->nSize >= size )
            return NULL;

        ptr.pBytes += ptr.pChunk->nSize;
    }
    return NULL;
}

/* Run-length encoded strange variable-length integer nightmare code */

static int WonkyNeilRaineCode( unsigned char **ppData )
{
    unsigned char *ptr = *ppData;
    int bit=0, result=0, byte;

    do {
        byte = *ptr++;
        result |= ( byte & 0x7F ) << bit;
        bit += 7;
    } while ( byte & 0x80 );

    *ppData = ptr;

    return result;
}


    /*=================*
     *   Constructor   *
     *=================*/


BOOL Anim_ConvertTCA( const void *data, size_t nSize,
                      anim_animfn animfn, anim_imagefn fn, void *handle )
{
    int log2bpp, bpp;
    tca_acef *pACEF;
    tca_pale *pPALE;
    tca_rate *pRATE;
    unsigned char *pFADE;
    int nFrames;
    tca_framepointer fptr;
    int xs, ys;
    int csDelay = 8;
    void *work;
    int bytewidth, abw;
    pixel *buffer;
    BOOL bDelta;
    int wkneeded;
    unsigned int nPixMask;
    int pause = 0, pausecount = 0;
    BOOL result = FALSE;
    anim_imageinfo pixels;

    /* Check it's an Animator file, if not return FALSE */

    if ( *(int*)data != chunkid_ACEF )
        return FALSE;

    pACEF = (tca_acef*) TCA_FindChunk( data, nSize, chunkid_ACEF );
    pPALE = (tca_pale*) TCA_FindChunk( data, nSize, chunkid_PALE );

    if ( !pACEF || !pPALE )
        return FALSE;

    log2bpp = pPALE->log2bpp;
    bpp = 1 << log2bpp;

    xs = pACEF->width >> pPALE->nXEIGFactor;
    ys = pACEF->height >> pPALE->nYEIGFactor;

    if ( animfn )
    {
        if ( !(*animfn)( handle, xs, ys, (pACEF->flags.data.bLoop ? 0 : 1) ) )
        {
            debugf( "converttca: animfn returned FALSE, exiting\n" );
            return FALSE;
        }
    }

    if ( bpp < 8 )
        nPixMask = (pixel)(pPALE->nPaper);
    else
        nPixMask = pPALE->palette[pPALE->nPaper];

    bytewidth = ( xs*bpp + 7 ) >> 3;
    abw = (bytewidth+3) & ~3;

    /* Buffer */

    buffer = Anim_Allocate( abw*ys );
    if ( !buffer )
    {
        Anim_NoMemory( "fromtca" );
        return FALSE;
    }

    /* Figure out the playback rate */

    pRATE = (tca_rate*) TCA_FindChunk( data, nSize, chunkid_RATE );
    if ( pRATE )
    {
        switch ( pRATE->tag )
        {
        case tcarate_CSPERFRAME:
            csDelay = pRATE->nSpeed;
            break;
        case tcarate_FRAMESPERSEC:
            csDelay = (int) ( 100.0 / pRATE->nSpeed );
            break;
        case tcarate_VSYNCSPERFRAME:
            csDelay = (int) ( pRATE->nSpeed * 100.0
                              / pRATE->nVSyncsPerSec );
        }
    }

    /* Pauses on individual frames */

    pFADE = (unsigned char*) TCA_FindChunk( data, nSize, chunkid_FADE );

    /* The frames */

    Workspace_Claim(0);

    fptr.pBytes = (char*) pACEF + pACEF->start;
    nFrames = 0;
    while ( *fptr.pWords )
    {
        bDelta = nFrames > 0
                 && ( pACEF->flags.data.bDelta );

        wkneeded = Riemann_DecompWspace( *fptr.pWords );

        /* Decompress the frame */

        debugf( "Decompressing frame %d\n", nFrames );

        work = Workspace_Claim( wkneeded );
        if ( !work )
        {
            Anim_NoMemory( "fromtca(2)" );
            goto err;
        }
        Riemann_Decompress( fptr.pBytes, abw, abw-bytewidth, ys, buffer, work,
                            bDelta );
        Workspace_Release();

        /* Is there a pause on this frame? */
        if ( pFADE && pausecount == 0 )
        {
            int fade;

            pausecount = *pFADE++;
            if ( pausecount )
            {
                pause = WonkyNeilRaineCode( &pFADE ) - 2;
                fade  = WonkyNeilRaineCode( &pFADE );

                if ( pause < 0 )
                    pause = 0;
            }
        }

        pixels.nWidth = xs;
        pixels.nLineWidthBytes = abw;
        pixels.nHeight = ys;
        pixels.nBPP = bpp;
        pixels.pBits = buffer;
        pixels.csDelay = csDelay + (pausecount ? pause : 0);

        if ( pausecount )
            pausecount--;

        if ( !(*fn)( handle, &pixels, nPixMask, NULL,
                     bpp<=8 ? pPALE->palette : NULL ) )
        {
            debugf( "converttca: fn returned FALSE, exiting\n" );
            goto err;
        }

        nFrames++;
        fptr.pBytes += *fptr.pWords;
    }

    result = TRUE;

err:
    Workspace_Release();
    Anim_Free( &buffer );
    return result;
}

/* eof */
