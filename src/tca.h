
/* The Complete Animator format definition for animlib/InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 */

/*=================================================*
 *   Parts of an Animator file                     *
 *   -- see http://www.iota.co.uk/info/animfile/   *
 *=================================================*/

#ifndef animlib_tca_h
#define animlib_tca_h

/* Compression types -- only LZW actually supported */
typedef enum {
    tcacompress_RLE_OBSOLETE = 0,
    tcacompress_LZW,
    tcacompress_NONE
} tca_compresstype;

/* Film flags */
typedef union {
    int value;
    struct {
        unsigned int bDelta : 1;
        unsigned int        : 1;
        unsigned int bLoop  : 1;    /* 0=loop 1=don't loop (!!!) */
        unsigned int bYoyo  : 1;
        unsigned int        : 28;
    } data;
} tca_filmflags;


/*----------------------------*
 * "ACEF" (image data) chunk  *
 *----------------------------*/

typedef struct {
    unsigned int film_length;
    char name[12];
    unsigned int start,
        width,
        height,
        mode;
    tca_compresstype compression;
    tca_filmflags flags;
    int unused[6];
} tca_acef;


/*-----------------------------------------*
 * "PALE" (palette and screen data) chunk  *
 *-----------------------------------------*/

typedef struct {
    int nPencil;
    int nPaper;
    int nModeFlags;
    int log2bpp;
    int log2bpc;
    int nXEIGFactor;
    int nYEIGFactor;
    unsigned int palette[1];    /* or more */
} tca_pale;


/* types of rate chunk */
typedef enum {
    tcarate_CSPERFRAME,
    tcarate_FRAMESPERSEC,
    tcarate_VSYNCSPERFRAME
} tcarate_tag;


/*-------------------------------*
 * "RATE" (playback rate) chunk  *
 *-------------------------------*/

typedef struct {
    tcarate_tag tag;
    int     nSpeed;
    int     nVSyncsPerSec;
} tca_rate;


/*---------------------------------------*
 * Useful typedefs for getting at chunks *
 *---------------------------------------*/

typedef struct {
    int chunkid;
    unsigned int nSize;
} tca_chunk;

typedef union {
    const tca_chunk *pChunk;
    const char *pBytes;
} tca_chunkpointer;

typedef union {
    char *pBytes;
    int *pWords;
} tca_framepointer;

#define chunkid_ACEF 0x46454341
#define chunkid_PALE 0x454C4150
#define chunkid_RATE 0x45544152
#define chunkid_FADE 0x45444146

#endif
