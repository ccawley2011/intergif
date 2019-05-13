/* cfsi.h */

/* Calling ChangeFSI from InterGif
 * (K) All Rites Reversed - Copy What You Like (see file Copying)
 *
 * Authors:
 *      Peter Hartley       <pdh@chaos.org.uk>
 */

BOOL ChangeFSI( const char *in, const char *out, const char *options );


#ifdef __acorn
BOOL MultiChangeFSI( const char *infile, const char *outfile, BOOL bJoin,
                     const char *options );
void MultiChangeFSI_RemoveScrapFiles( const char *infile );
#endif
