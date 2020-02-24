/* Utils.h */

/* OS dependency layer and other internal bits for animlib/InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 */

#ifndef animlib_utils_h
#define animlib_utils_h

#ifndef animlib_h
#include "animlib.h"
#endif

#ifndef BOOL
#define BOOL int
#define TRUE 1
#define FALSE 0
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

/* File handling (utils.c) */

int  Anim_FileSize( const char *filename );
BOOL Anim_LoadFile( const char *filename, void *pData );
#define filetype_GIF 0x695
#define filetype_SPRITE 0xFF9
void Anim_SetFileType( const char *filename, int type );


/* Memory handling (utils.c) */

void *Anim_Allocate( int nSize );
void *Anim_Reallocate( void *pBlock, int nSize );
void  Anim_Free( void *ppBlock );

void  Anim_CheckHeap( char*, int );


/* Error handling (anim.c) */

void Anim_SetError( const char *report, ... );
void Anim_NoMemory( const char *where );


/* Pixel handling (from.c) */

/* Convert %0bbbbbgggggrrrrr to &BBGGRR00 */
unsigned int Anim_Pix16to32(unsigned int x);


/* Filename handling (split.c) */

void Anim_NthName( const char *in, char *out, int n );

/* Let the user know something's happening */

void Anim_Percent( int percent );

void Anim_Debug( const char *file, const char *func, int line, const char *format, ...);
#if DEBUG
#define debugf(...) Anim_Debug(__FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define debugf(...) do { } while (0)
#endif

#endif
