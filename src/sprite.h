/* Sprite.h */

/* RiscOS sprite format definition for animlib/InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 */

/* Parts of a sprite file (like Desklib:Sprite.h, except that some releases
 * of DeskLib get this wrong, and define a sprite as a sprite_info* rather
 * than a sprite_header*). Also we can't exactly require Windows and Linux
 * users to install DeskLib just to compile InterGif...
 */

#ifndef animlib_sprite_h
#define animlib_sprite_h

#ifndef animlib_h
#include "animlib.h"
#endif

typedef struct
{
	BOOL isType  : 1;
	unsigned int horz_dpi : 13;
	unsigned int vert_dpi : 13;
	unsigned int type     : 4;
	BOOL isWide  : 1;
} modesprite;

typedef union
{
	unsigned int screen_mode;
	modesprite sprite_mode;
} modeval;

typedef struct {
    unsigned int nSize;
    unsigned int nSprites;
    unsigned int nFirstOffset;
    unsigned int nFreeOffset;
} spriteareastr;

typedef spriteareastr *spritearea;

typedef struct
{
  int  nNextOffset;
  char name[12];
  int  width;
  int  height;
  unsigned int  leftbit;
  unsigned int  rightbit;
  int  imageoffset;
  int  maskoffset;
  modeval screenmode;
} spritestr;

typedef spritestr *sprite;

typedef struct {
  char  ModeFlags;
  char  ScrRCol;
  char  ScrBRow;
  char  NColour;
  char  XEigFactor;
  char  YEigFactor;
  short LineLength;
  int   ScreenSize;
  char  YShftFactor;
  char  Log2BPP;
  char  Log2BPC;
  short XWindLimit;
  short YWindLimit;
} mode_variables;

extern const mode_variables riscos_modes[50];

/* Is this a default RiscOS palette? (loadsprite.c) */
BOOL Sprite_IsDefaultPalette( const unsigned int *pPalette,
                              unsigned int nColours );

void Sprite_CreateDefaultPalette( int ncol, unsigned int *pPalette );

#endif
