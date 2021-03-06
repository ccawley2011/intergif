/* Join.c */

/* Making an anim that takes one frame each from lots of files (-join option)
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Peter Hartley       <pdh@chaos.org.uk>
 *
 * History:
 *      27-Nov-96 pdh Created (as joinanim.c)
 *      15-Dec-96 *** Release 5beta1
 *      27-Jan-97 *** Release 5beta2
 *      29-Jan-97 *** Release 5beta3
 *      03-Feb-97 *** Release 5
 *      07-Feb-97 *** Release 5.01
 *      10-Mar-97 pdh Frob for new anim library
 *      07-Apr-97 *** Release 6beta1
 *      20-May-97 pdh Fix very dumb realloc bug
 *      20-May-97 *** Release 6beta2
 *      24-Aug-97 *** Release 6
 *      27-Sep-97 *** Release 6.01
 *      08-Nov-97 *** Release 6.02
 *      21-Feb-98 *** Release 6.03
 *      07-Jun-98 pdh Fix bug with joining deep sprites
 *      07-Jun-98 *** Release 6.04
 *      21-Aug-98 *** Release 6.05
 *      05-Oct-98 *** Release 6.06
 *      19-Feb-99 *** Release 6.07
 *      26-Mar-00 *** Release 6.10
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "animlib.h"
#include "cfsi.h"

#include "utils.h"


BOOL Anim_ConvertFiles( const char *first,
                        anim_animfn animfn, anim_imagefn fn, void *handle )
{
    char buffer[256];
    int n = 1;
    BOOL result;

    result = Anim_ConvertFile( first, animfn, fn, handle );

    while ( result )
    {
        Anim_NthName( first, buffer, n++ );
        if ( Anim_FileSize( buffer ) <= 0 )
            break;

        debugf( "anim_fromfiles loads %s\n", buffer );

        result = Anim_ConvertFile( buffer, animfn, fn, handle );
    }

    return result;
}

BOOL Anim_ConvertList( const char *listfile,
                       anim_animfn animfn, anim_imagefn fn, void *handle )
{
    char buffer[256];
    BOOL result = TRUE;

    FILE* list;

    if ( (list = fopen( listfile, "rb" ) ) == NULL)
    {
        Anim_SetError( "Could not open list file %s\n", listfile );
        return FALSE;
    }

    while ( fgets(buffer, 256, list) && result )
    {
        if ( Anim_FileSize( buffer ) <= 0 )
            break;

        debugf( "anim_fromfiles loads %s\n", buffer );

        result = Anim_ConvertFile( buffer, animfn, fn, handle );
    }

    fclose( list );
    return result;
}

/* eof */
