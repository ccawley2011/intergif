/* ColMatch.c */

/* Routines for finding closest colours
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Peter Hartley       <pdh@chaos.org.uk>
 *      Martin W�rthner
 *
 * History:
 *      01-Mar-97 pdh Created
 *      07-Apr-97 *** Release 6beta1
 *      20-May-97 *** Release 6beta2
 *      24-Aug-97 *** Release 6
 *      27-Sep-97 *** Release 6.01
 *      08-Nov-97 *** Release 6.02
 *      21-Feb-98 *** Release 6.03
 *      07-Jun-98 *** Release 6.04
 *      21-Aug-98 *** Release 6.05
 *      05-Oct-98 *** Release 6.06
 *      19-Feb-99 *** Release 6.07
 *      26-Mar-00 *** Release 6.10
 *      18-Aug-00 mw  Fix distance metric
 *      28-Aug-00 *** Release 6.11
 *
 */

#include <string.h>
#include <stdarg.h>

#include "animlib.h"
#include "utils.h"

typedef struct colourmatchstr {
    unsigned int nColours;
    unsigned int *pColours;
    unsigned int mdist[1];      /* or more */
} colourmatchstr;

static int Distance( unsigned int c1, unsigned int c2 )
{
    int delta_b, delta_g, delta_r;

    delta_b = ((c1>>24)&0xFF) - ((c2>>24)&0xFF);        /* b */

    delta_g = ((c1>>16)&0xFF) - ((c2>>16)&0xFF);        /* g */

    delta_r = ((c1>>8)&0xFF) - ((c2>>8)&0xFF);          /* r */

    /* no fear for overflow because the deltas are all in the range
     * [-255 ... +255]
     */
    return 3 * delta_r * delta_r + 10 * delta_g * delta_g + delta_b * delta_b;
}

colourmatch ColourMatch_Create( unsigned int n, unsigned int *p )
{
    colourmatch res = Anim_Allocate( sizeof(colourmatchstr)
                                      + (n-1)*sizeof(unsigned int) );
    unsigned int i,j;

    if ( !res )
    {
        Anim_NoMemory( "cmatchnew" );
        return NULL;
    }

    res->nColours = n;
    res->pColours = p;

    for ( i=0; i<n; i++ )
    {
        res->mdist[i] = 0x40000000;
    }

    for ( i=0; i<n-1; i++ )
    {
        unsigned int pi = p[i];

        for ( j=i+1; j<n; j++ )
        {
            unsigned int mdist = Distance( pi, p[j] )/2;

            if ( mdist < res->mdist[j] )
                res->mdist[j] = mdist;
            if ( mdist < res->mdist[i] )
                res->mdist[i] = mdist;
        }
    }

#if DEBUG
    debugf( "Palette:" );
    for ( i=0; i<n; i++ )
    {
        debugf( " <%08x>", res->pColours[i] );
    }
    debugf( "\n" );

    debugf( "Mindists:\n" );
    for ( i=0; i<n; i++ )
    {
        debugf( " %d", res->mdist[i] );
    }
    debugf( "\n" );
#endif

    return res;
}

unsigned int ColourMatch_Match( colourmatch cm, unsigned int c, BOOL bIndex )
{
    unsigned int i;
    unsigned int n = cm->nColours;
    const unsigned int *p = cm->pColours;
    const unsigned int *mdist = cm->mdist;
    unsigned int mindist = 0x40000000;
    pixel best = 0;
    unsigned int dist;

    for ( i=0; i < n; i++ )
    {
        dist = Distance( c, p[i] );

        if ( dist < mindist )
        {
            if ( dist < mdist[i] )
            {
                debugf( "match(0x%08X): returning 0x%08X dist=%d mdist=%d\n",
                        c, p[i], dist, mdist[i] );
                return bIndex ? i : p[i];
            }
            best = (pixel)i;
            mindist = dist;
        }
    }
    debugf( "match(0x%08X): returning %03d/%03d 0x%08X dist=%d\n", c, best, n, p[best], mindist );

#if 0
    if ( mindist > 3000 )
    {
        debugf("pal =");
        for (i=0; i<n; i++)
            debugf(" <%08x>", p[i]);
        debugf("\n");
    }
#endif

    return bIndex ? best : p[best];
}

void ColourMatch_Destroy( colourmatch *pcm )
{
    Anim_Free( pcm );
}

/* eof */
