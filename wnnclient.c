/****************************************************************************
 * wnnclient.c                                                              *
 * Interface to the IME engine (FreeWnn).                                   *
 * NOTE: This module must be compiled with _cdecl linkage (with ICC: /Mc).  *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  This program is free software; you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation; either version 2 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program; if not, write to the Free Software             *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA                *
 *  02111-1307  USA                                                         *
 *                                                                          *
 ****************************************************************************/
#define INCL_DOSERRORS
#define INCL_WINATOM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <uconv.h>
#include <wnn/jllib.h>
#include <wnn/rk_spclval.h>

#include "codepage.h"
#include "wnnhook.h"
#include "wnnim.h"
#include "wnnclient.h"



// (In wnnconv.c) Convert between fixed-width 2-byte EUC and normal (packed) EUC
int _cdecl wnn_sStrcpy ( register char *c, register w_char *w );
int _cdecl wnn_Sstrcpy ( w_char *w, unsigned char *c );



// Romkan API functions exported by WNN library (just the ones we actually use).
//

extern void     _cdecl romkan_set_lang( char * );
extern int      _cdecl romkan_init( char*, letter, char, letter (* _cdecl keyinfn)(),int (* _cdecl bytcntfn)() );
// extern letter   _cdecl romkan_getc( void );
extern letter * _cdecl romkan_henkan( letter );
extern void     _cdecl romkan_clear( void );



// JLIB and other API functions exported by WNN library.  (Again, this is not
// comprehensive, just the ones we need.)
//
extern struct wnn_buf * _cdecl jl_open_lang( const char *, const char *, const char *, const char *, int (* _cdecl)(const char *), int (* _cdecl)(const char *), int );
extern int              _cdecl jl_isconnect_e( register struct wnn_env *env );
extern struct wnn_env * _cdecl jl_env_get( register struct wnn_buf *buf );
extern int              _cdecl jl_fuzokugo_get_e( register struct wnn_env *env, char *fname );
extern int              _cdecl jl_set_env_wnnrc( register struct wnn_env *, const char *, int (* _cdecl)(const char *), int (* _cdecl)(const char *) );
extern int              _cdecl jl_fuzokugo_set_e( struct wnn_env *env, char *fname );
extern void             _cdecl jl_close( register struct wnn_buf *buf );
extern int              _cdecl jl_dic_list_e( struct wnn_env *env, WNN_DIC_INFO **dicinfo );
extern int              _cdecl jl_kanji_len( struct wnn_buf *buf, register int bun_no, register int bun_no2 );
extern int              _cdecl jl_kill( struct wnn_buf *buf, register int bun_no, register int bun_no2 );
extern int              _cdecl jl_ren_conv( register struct wnn_buf *buf, register w_char *yomi, int bun_no, int bun_no2, int use_maep );
extern int              _cdecl jl_tan_conv( register struct wnn_buf *buf, w_char *yomi, register int bun_no, register int bun_no2, int use_maep, int ich_shop );
extern int              _cdecl jl_yomi_len( struct wnn_buf *buf, register int bun_no, register int bun_no2 );
extern int              _cdecl jl_zenkouho( register struct wnn_buf *buf, int bun_no, int use_maep, int uniq_level );
extern int              _cdecl jl_zenkouho_dai( register struct wnn_buf *buf, int bun_no, int bun_no2, int use_maep, int uniq_level );
extern int              _cdecl wnn_get_area( struct wnn_buf *buf, register int bun_no, register int bun_no2, w_char *area, int kanjip );


// --------------------------------------------------------------------------

extern IMCLIENTDATA global;

char   WnnErrorBuf[ 128 ] = {0};
BOOL   fInitRK            = FALSE;
BOOL   fInitCJK           = FALSE;
USHORT usCharIdx;                // index of next input character to be converted

UconvObject uconvEUC   = NULL;   // conversion object (EUC to UCS-2)
XformObject xfKatakana = NULL;   // transformation object for fullwidth katakana


// =========================================================================
// CALLBACK FUNCTIONS
// =========================================================================

/* ------------------------------------------------------------------------- *
 * NextCharacter                                                             *
 *                                                                           *
 * Callback function registered with romkan_init(): returns the next input   *
 * byte for conversion.  The FreeWnn romkan_next() function (which is called *
 * internally by romkan_getc()) uses this to retrieve each byte.             *
 *                                                                           *
 * Since we don't use romkan_getc(), this is presumably unused in practice.  *
 * ------------------------------------------------------------------------- */
letter _cdecl NextCharacter()
{
    letter ltr;
    if ( usCharIdx >= sizeof( global.szRomaji ))
        ltr = EOLTTR;
     else
        ltr = global.szRomaji[ usCharIdx ] ?
                  (letter)( global.szRomaji[ usCharIdx++ ]) :
                  EOLTTR;
    return ltr;
}


/* ------------------------------------------------------------------------- *
 * CharacterByteCount                                                        *
 *                                                                           *
 * Callback function registered with romkan_init(): returns whether an       *
 * input character value is a single- or double-byte character for the       *
 * active input codepage.  Used by romkan_next() via romkan_getc().          *
 *                                                                           *
 * Since we don't use romkan_getc(), this is presumably unused in practice.  *
 * ------------------------------------------------------------------------- */
int _cdecl CharacterByteCount( char *pChar )
{
    return 1;       // We only pass ASCII characters as input anyway, so...
}


/* ------------------------------------------------------------------------- *
 * ErrorFunc                                                                 *
 *                                                                           *
 * Callback function registered with jl_open_lang(): used to output error    *
 * messages.                                                                 *
 * ------------------------------------------------------------------------- */
int _cdecl ErrorFunc( const char *pcsz )
{
    if ( pcsz && *pcsz )
        strncpy( WnnErrorBuf, pcsz, sizeof( WnnErrorBuf ));
    else
        WnnErrorBuf[ 0 ] = 0;
    return 1;
}



// =========================================================================
// OTHER FUNCTIONS
// =========================================================================

/* ------------------------------------------------------------------------- *
 * InitConversionMethod                                                      *
 *                                                                           *
 * Initialize the CJK conversion engine.  In this implementation (FreeWnn),  *
 * this involves connecting to the FreeWnn server and initializing the       *
 * associated environment.                                                   *
 *                                                                           *
 * ------------------------------------------------------------------------- */
INT _Optlink InitConversionMethod( USHORT usLang, PVOID *ppSession )
{
    struct wnn_env *wnnenv = NULL;  // Wnn environment object
    struct wnn_buf *bdata = NULL;   // Wnn session buffer

    PSZ     pszEnv,                 // Return pointer for getenv()
            pszServer,              // Host address of jserver
            pszUser;                // User/environment name to use on the server
    CHAR    szLang[ 6 ];
    CHAR    fzk[ 1024 ] = {0};
    INT     result = 0;

    if ( !fInitRK ) return 1;
    if ( uconvEUC == NULL ) return 2;

    // Get the server and user names to use.
    switch ( usLang ) {
        default:
        case MODE_JP:
            pszEnv = getenv( WNN_SERVER_ENV_JA );
            strcpy( szLang, "ja_JP");
            break;
        case MODE_KR:
            pszEnv = getenv( WNN_SERVER_ENV_KR );
            strcpy( szLang, "ko_KR");
            break;
        case MODE_CN:
            pszEnv = getenv( WNN_SERVER_ENV_CN );
            strcpy( szLang, "zh_CN");
            break;
        case MODE_TW:
            pszEnv = getenv( WNN_SERVER_ENV_TW );
            strcpy( szLang, "zh_TW");
            break;
    }
    pszServer = strdup( pszEnv? pszEnv: "localhost");
    pszEnv = getenv("USER");
    pszUser = strdup( pszEnv? pszEnv: "root");

    // Connect to the server.
    bdata = jl_open_lang( pszUser, pszServer, szLang, NULL, *ErrorFunc, *ErrorFunc, 0 );
    if (( bdata == NULL ) || ( jl_isconnect( bdata ) == 0 )) {
        if ( WnnErrorBuf[0] )
            strncpy( global.szEngineError, WnnErrorBuf, sizeof( global.szEngineError ) - 1 );
        result = 1;
        goto done_connect;
    }

    if ( jl_fuzokugo_get( bdata, fzk ) == -1 ) {
        //  Environment isn't active on server, so initialize it now.
        wnnenv = jl_env_get( bdata );
        jl_set_env_wnnrc( wnnenv, "wnnenvrc", (int *) WNN_CREATE, NULL );
    }

    *ppSession = bdata;
    fInitCJK = TRUE;

done_connect:
    free( pszServer );
    free( pszUser );
    return result;
}


/* ------------------------------------------------------------------------- *
 * FinishConversionMethod                                                    *
 *                                                                           *
 * Close down the clause conversion IME engine and free any associated       *
 * resources.                                                                *
 * ------------------------------------------------------------------------- */
void _Optlink FinishConversionMethod( PVOID pSession )
{
    struct wnn_buf *bdata = pSession;

    if ( bdata && jl_isconnect( bdata )) {
        // Free anything left over in the conversion buffer.
        if ( jl_bun_suu( bdata ))
            jl_kill( bdata, 0, -1 );

        // Now close the connection (also frees the buffer)
        if ( jl_isconnect( bdata ))
            jl_close( bdata );
    }
}


/* ------------------------------------------------------------------------- *
 * InitInputMethod                                                           *
 *                                                                           *
 * Initialize the phonetic input conversion engine.  In this implementation  *
 * (FreeWnn romkan), this involves loading the romkan table files.           *
 *                                                                           *
 * ------------------------------------------------------------------------- */
INT _Optlink InitInputMethod( PSZ pszPath, USHORT usLang )
{
    USHORT cpEUC;
    CHAR   szLang[ 6 ];
    CHAR   szModeHyo[ CCHMAXPATH ];
    int    rc;


    switch ( usLang ) {
        case MODE_CN: strcpy( szLang, "zh_CN"); break;
        case MODE_TW: strcpy( szLang, "zh_TW"); break;
        case MODE_KR: strcpy( szLang, "ko_KR"); break;
        default:      strcpy( szLang, "ja_JP"); break;
    }

    if ( pszPath == NULL ) {
        pszPath = getenv("ROMKAN_TABLE");
        if ( pszPath )
            strncpy( szModeHyo, pszPath, CCHMAXPATH - 1 );
        else
            sprintf( szModeHyo, "/@unixroot/usr/lib/Wnn/%s/rk/mode", szLang );
        pszPath = szModeHyo;
    }

    romkan_set_lang( szLang );     // This may not actually be needed (?)

    // romkan_init() parameters:
    //   pszPath                Filespec of the main 'mode' table
    //   0x08                   Value of the 'delete' character code
    //   *NextCharacter         Pointer to character-read function
    //   *CharacterByteCount    Pointer to character byte-count function

    rc = romkan_init( pszPath, 0x08, 0, *NextCharacter, *CharacterByteCount );

    if ( rc )
        sprintf( global.szEngineError, "Failed to initialize romkan (Wnn) library (error %u)", rc );
    fInitRK = ( rc == 0 )? TRUE: FALSE;

    // Create a conversion object for EUC encoding (what Wnn returns its strings in)
    if ( fInitRK && ( uconvEUC == NULL )) {
        cpEUC = GetEucCodepage( usLang );
        rc = CreateUconvObject( cpEUC, &uconvEUC );
        if ( rc )
            sprintf( global.szEngineError, "Failed to create conversion object for codeoage %u (error %u). The OS/2 codepage file might not be installed.", cpEUC, rc );
    }

    if (( rc == NO_ERROR ) && ( usLang == MODE_JP )) {
        // Create transformation objects for alternate Japanese kana modes
        if ( UniCreateTransformObject( NULL, (UniChar *)L"katakana", &xfKatakana ) != NO_ERROR )
            xfKatakana = NULL;
    }

    return rc;
}


/* ------------------------------------------------------------------------- *
 * FinishInputMethod                                                         *
 *                                                                           *
 * Close down the input method engine and free any associated resources      *
 * (romkan doesn't utilize any cleanup functions so there's not much to do). *
 * ------------------------------------------------------------------------- */
void _Optlink FinishInputMethod( void )
{
    if ( uconvEUC != NULL )
        UniFreeUconvObject( uconvEUC );
    if ( xfKatakana != NULL )
        UniFreeTransformObject( xfKatakana );
}


/* ------------------------------------------------------------------------- *
 * StrTransform                                                              *
 *                                                                           *
 * Applies a ULS transformation to a UCS-2 string.                           *
 * ------------------------------------------------------------------------- */
INT _Optlink StrTransform( UniChar *puszString, INT iMax, XformObject xform )
{
    int iLen,
        iOut,
        rc;
    UniChar *puszTemp;

    if ( puszString == NULL ) return 0;

    iLen = UniStrlen( puszString );
    iOut = iMax;
    puszTemp = (UniChar *) calloc( iMax, sizeof( UniChar ));
    if ( !puszTemp ) return ERROR_NOT_ENOUGH_MEMORY;

    rc = UniTransformStr( xform, puszString, &iLen, puszTemp, &iOut );
    if ( rc == ULS_SUCCESS )
        UniStrncpy( puszString, puszTemp, iMax - 1 );
    free( puszTemp );

    return rc;
}


/* ------------------------------------------------------------------------- *
 * MakeKatakana                                                              *
 *                                                                           *
 * Transforms an already-converted kana string into fullwidth katakana.      *
 * ------------------------------------------------------------------------- */
INT _Optlink MakeKatakana( void )
{
    StrTransform( global.uszKana, sizeof( global.uszKana ), xfKatakana );
    return 0;
}


/* ------------------------------------------------------------------------- *
 * MakeHalfKana                                                              *
 *                                                                           *
 * Transforms an already-converted kana string into halfwidth katakana.      *
 * ------------------------------------------------------------------------- */
INT _Optlink MakeHalfKana( void )
{
    // TODO

    return 0;
}


/* ------------------------------------------------------------------------- *
 * PreprocessKana                                                            *
 *                                                                           *
 * This function checks for and converts certain kana sequences not handled  *
 * by romkan_henkan()'s normal hiragana logic.  This is done before the call *
 * to romkan_henkan(), and thus also prior to the output buffer's conversion *
 * from EUC to UCS-2.  Therefore, this function generates EUC output.        *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   USHORT fsMode   : The current input mode flags.                         *
 *   PUSHORT pusIn   : Current position in the input buffer.                 *
 *   PUSHORT pusOut  : Current position in the output buffer.                *
 *   PSZ    pszOutput: Pointer to the output buffer.                         *
 *   USHORT cbOutput : Total size of the output buffer, in bytes.            *
 *                                                                           *
 * RETURNS: BYTE                                                             *
 * Result of the conversion attempt.  This will be either KANA_COMPLETE      *
 * (meaning we found & converted a sequence) or KANA_PENDING (in which case  *
 * the calling function will proceed to use romkan_henkan() as usual).       *
 * ------------------------------------------------------------------------- */
BYTE _Optlink PreprocessKana( USHORT fsMode, PUSHORT pusIn, PUSHORT pusOut, PSZ pszOutput, USHORT cbOutput )
{

#define NUM_SPEC_KATAKANA 27

    // Special katakana sequences (romaji input)
    CHAR *aszSpcKataIn[] = {
        "KWA", "GWA",
        "SHE", "JE",  "CHE",
        "TSA", "TSE", "TSO",
        "TI",  "XTU", "TU",
        "DI",  "DYU", "DU",
        "FA",  "FI",  "FE",  "FO",
        "WI",  "WE",  "WO",  "YE",
        "VA",  "VI",  "VU",  "VE",  "VO"
    };
    // Special katakana sequences (kana output); these are EUC-JP encoded
    CHAR *aszSpcKataOut[] = {
        "クァ", "グァ",
        "シェ", "ジェ", "チェ",
        "ツァ", "ツェ", "ツォ",
        "ティ", "ッ",   "トゥ",
        "ディ", "デュ", "ドゥ",
        "ファ", "フィ", "フェ", "フォ",
        "ウィ", "ウェ", "ウォ", "イェ",
        "ヴァ", "ヴィ", "ヴ",   "ヴェ", "ヴォ"
    };

    USHORT index;
    PSZ    pszInput;
    BYTE   result = KANA_PENDING;

    if ( IS_INPUT_MODE( fsMode, MODE_KATAKANA )) {
        pszInput = (PSZ)(global.szRomaji) + *pusIn;
        for ( index = 0; index < NUM_SPEC_KATAKANA; index++ ) {
            if ( strcmpi( pszInput, aszSpcKataIn[ index ] ) == 0 ) {
                strncat( pszOutput, aszSpcKataOut[ index ], cbOutput );
                *pusIn  += strlen( aszSpcKataIn[ index ] );
                *pusOut += strlen( aszSpcKataOut[ index ] );
                result = KANA_COMPLETE;
                break;
            }
        }
    }
    return result;
}


/* ------------------------------------------------------------------------- *
 * ConvertPhonetic                                                           *
 *                                                                           *
 * Convert the input ('romaji') buffer into phonetic characters ('kana') for *
 * the current language.  (We use the term 'kana', which is Japanese, but    *
 * this also applies to Korean Hangul.)                                      *
 *                                                                           *
 * Note that the converted result may consist of several bytes, possibly     *
 * even more than two in the case of composite characters.                   *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   USHORT fsMode: The current input mode flags.                            *
 *                                                                           *
 * RETURNS: BYTE                                                             *
 * Result of the conversion attempt.  One of:                                *
 *   KANA_INVALID   Invalid character sequence, not converted; caller can do *
 *                  what it likes with it, but should clear the buffers.     *
 *   KANA_PENDING   Incomplete character sequence, not converted; caller     *
 *                  should keep the input buffer as-is, and continue.        *
 *   KANA_COMPLETE  Valid character sequence, successfully converted; caller *
 *                  should process the converted text & clear the buffers.   *
 *   KANA_CANDIDATE Valid & successfully converted, may be modified later;   *
 *                  caller should allow the user to confirm the current      *
 *                  converted value (in which case process it and clear the  *
 *                  buffers), or continue to add characters (in which case   *
 *                  the caller should keep the input buffer and continue).   *
 * ------------------------------------------------------------------------- */
BYTE _Optlink ConvertPhonetic( USHORT fsMode )
{
    CHAR   szOutput[ 8 ];       // should be big enough for any known sequence
    USHORT i, j,                // index variables
           len;                 // input string length
    BYTE   result;              // result to return
    letter ltr,                 // current letter being examined
           *converted,          // pointers to converted letters
           *c;                  // "

//FILE *f;


    if ( !fInitRK ) return KANA_INVALID;

//f = fopen("debug.log", "a");

    i = 0;
    ltr = 0;
    result = KANA_PENDING;
    len = strlen( global.szRomaji );

    memset( szOutput, 0, sizeof( szOutput ));

    // Read (and try to convert) the input buffer until either we reach the
    // end, or the current state becomes something other than KANA_PENDING.

    // NOTE: We don't use romkan_getc() due to problems with its internal loops.
    // Instead, we call romkan_henkan() and handle the iteration ourselves.

    for( j = 0; (result == KANA_PENDING) && (j < len); j++ ) {

//fprintf( f, "romaji: %s, start: %d, converted: %d\t", global.szRomaji, j, i );

        // Get the next input character
        ltr = (letter)(global.szRomaji[ j ]);

        if ( ltr && ( fsMode & MODE_JP )) {
            // Check for kana that require special handling.  If found, the
            // buffer positions will be advanced by this function.
            result = PreprocessKana( fsMode, &i, &j, szOutput, sizeof( szOutput ));
            ltr = (letter)(global.szRomaji[ j ]);
        }

        if (( ltr == 0 ) || ( i >= sizeof( szOutput ) - 1 )) {
            // End of input string
            converted = NULL;
            ltr = LTREOF;
        }

#if 0   // not actually needed at the moment
        else if ( IsDBCSLeadByte( (CHAR)(ltr & 0xFF), global.dbcs )) {
            // This is already a DBCS character so don't try and convert it;
            // just read both bytes and continue.
            szOutput[ i++ ] = (CHAR)(ltr & 0xFF);
            ltr = (letter)(global.szRomaji[ ++j ]);
            if (( ltr == 0 ) || ( i >= sizeof( szOutput ) - 1 ))
                ltr = LTREOF;
            else
                szOutput[ i++ ] = (CHAR)(ltr & 0xFF);
        }
#endif

        else if ( result == KANA_PENDING ) {
            // Input character that needs converting.  Note that romkan_henkan()
            // converts ltr into (potentially) a series of output bytes, so
            // we have to read all of them...
            converted = romkan_henkan( ltr & 0xFF );
            c = converted;
            while (( *c != EOLTTR ) && i < sizeof( szOutput )) {
                ltr = *c;
                if ( is_HON( ltr )) {
                    szOutput[ i++ ] = (CHAR)(ltr & 0xFF);
                    result = KANA_COMPLETE;
                }
                c++;
                if (( c - converted ) > 10 ) break;     // simple sanity check
            }
        }
//fprintf( f, " (%08X)\n", ltr );

        if ( ltr == NISEBP ) {
            result = KANA_INVALID;
            break;
        }
        else if ( ltr == EOLTTR || ltr == LTREOF )
            break;           // end of input, exit with the last status result
        else if ( ltr & 0x80000000 )
            result = KANA_PENDING;
        else
            break;          // exit with the last status result
        // TODO not sure how to identify KANA_CANDIDATE yet (only used for Korean)
    }

    // Convert szOutput to UCS-2
    if ( i ) {
        StrConvert( szOutput, (PCH)(global.uszKana), uconvEUC, NULL );

        // Apply any special transformations (e.g. hiragana to katakana)
        if ( IS_LANGUAGE( fsMode, MODE_JP )) {
            if ( IS_INPUT_MODE( fsMode, MODE_KATAKANA ))
                MakeKatakana();
            else if ( IS_INPUT_MODE( fsMode, MODE_HALFWIDTH ))
                MakeHalfKana();
        }

//fprintf( f, "Converted: %s\n", szOutput );
    }
    romkan_clear();

//fprintf( f, "Returning result: %d\n", result );

//fclose( f );

    return result;
}


/* ------------------------------------------------------------------------- *
 * ConvertClause                                                             *
 *                                                                           *
 * Convert a phonetic character string (clause) into CJK ideographic text    *
 * for the current language.  The input is taken from the global.uszKana     *
 * buffer; the output is (for now) saved in the Wnn buffer (pSession).  If   *
 * the conversion was successful, the client can subsequently retrieve the   *
 * candidates for the full converted clause, and/or split the clause into    *
 * individual phrases and get the candidates for each phrase.                *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   PVOID pSession: Pointer to Wnn data buffer.                             *
 *                                                                           *
 * RETURNS:                                                                  *
 * Result of the conversion attempt.  One of:                                *
 *   CONV_CONNECT  No connection to server.                                  *
 *   CONV_FAILED   Conversion failed.                                        *
 *   CONV_OK       Conversion succeeded.                                     *
 * ------------------------------------------------------------------------- */
BYTE _Optlink ConvertClause( PVOID pSession )
{
    CHAR szTemp[ MAX_KANA_BUFZ * 3 ] = {0};     // Temporary conversion buffer
    INT             iLen,                       // Buffer length
                    iResult;                    // Conversion result code from jlib
    w_char         *yomi;                       // Input string in fixed-width EUC format
    struct wnn_buf *bdata = pSession;           // Wnn session buffer


    // Double-check we are connected to the server
    if ( ! jl_isconnect( bdata )) {
        strcpy( global.szEngineError, "Lost connection to server.");
        return CONV_CONNECT;
    }

    // Convert the UCS-2 kana string into EUC
    StrConvert( (PCH)(global.uszKana), szTemp, NULL, uconvEUC );

    // Now convert that into the fixed-width format expected by Wnn jlib
    iLen = strlen( szTemp ) + 1;
    yomi = (w_char *) calloc( iLen, sizeof( w_char ));
    wnn_Sstrcpy( yomi, szTemp );

    // Tell Wnn to convert the full clause
    iResult = jl_ren_conv( bdata, yomi, 0, -1, WNN_USE_MAE );
    if ( iResult == -1 ) {
        if ( WnnErrorBuf[0] )
            strncpy( global.szEngineError, WnnErrorBuf, sizeof( global.szEngineError ) - 1 );
        else
            strcpy( global.szEngineError, "Error converting text.");
        return CONV_FAILED;
    }

    free( yomi );
    return CONV_OK;
}



/* ------------------------------------------------------------------------- *
 * Get a converted kanji string from the Wnn buffer and return it in a       *
 * displayable format.  This could be a phrase in the unconverted clause     *
 * (for which specify a phrase range with fReading = TRUE), or a candidate   *
 * conversion result for the whole clause (iPhrase = 0, iCount = -1) or a    *
 * word phrase range.                                                        *
 *                                                                           *
 * NOTES:                                                                    *
 * - Before any of this can work, jl_ren_conv must have been called on the   *
 *   the clause buffer (i.e. using ConvertClause).                           *
 * - Obtaining an individual phrase also requires jl_tan_conv to have been   *
 *   called subsequently (i.e. using PreparePhrases).                        *
 * - To obtain a conversion candidate, jl_zenkouhou and jl_next/jl_previous  *
 *   must then have been called as well (i.e. using PrepareCandidates and    *
 *   SetCandidate).                                                          *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   PVOID pSession: Pointer to Wnn data buffer.                             *
 *   INT   iPhrase : First phrase in the clause to retrieve (first = 0)      *
 *   INT   iCount  : Number of phrases to retrieve (-1 for rest of clause)   *
 *   BOOL  fReading: If TRUE, return unconverted reading instead of kanji.   *
 *   UniChar **ppuszString: Pointer to UCS-2 string containing the result.   *
 *                          String will be allocated by this function and    *
 *                          should be free()d by the caller.                 *
 *                                                                           *
 * RETURNS:                                                                  *
 * One of:                                                                   *
 *   CONV_CONNECT  No connection to server.                                  *
 *   CONV_FAILED   Failed to retrieve string.                                *
 *   CONV_OK       String retrieved successfully.                            *
 * ------------------------------------------------------------------------- */
BYTE _Optlink GetConvertedString( PVOID pSession, INT iPhrase, INT iCount, BOOL fReading, UniChar **ppuszString )
{
    INT     iLen;                       // Buffer length
    BYTE    bResult = CONV_FAILED;      // Return code from this function
    PSZ     pszEUC;                     // Converted kanji string in standard EUC format
    w_char *kanji;                      // Converted string in fixed-width EUC format
    struct wnn_buf *bdata = pSession;   // Wnn session buffer


    // Double-check we are connected to the server
    if ( ! jl_isconnect( bdata )) {
        strcpy( global.szEngineError, "Lost connection to server.");
        return CONV_CONNECT;
    }

    // Get the requested string length
    iLen = fReading? jl_yomi_len( bdata, iPhrase, iCount ) :
                     jl_kanji_len( bdata, iPhrase, iCount );
    if ( !iLen ) return bResult;

    // Allocate a buffer for the string
    kanji = (w_char *) calloc( iLen+1, sizeof( w_char ));
    if ( !kanji ) return bResult;

    // Now retrieve the requested string or substring(s)
    iLen = fReading? jl_get_yomi( bdata, iPhrase, iCount, kanji ) :
                     jl_get_kanji( bdata, iPhrase, iCount, kanji );
    if ( iLen > 0 ) {
        // Convert to standard EUC, allowing up to 3 bytes per input character
        iLen *= 3;
        pszEUC = (PSZ) calloc( iLen+1, sizeof( CHAR ));
        if ( !pszEUC ) goto done;
        wnn_sStrcpy( pszEUC, kanji );
        // Now convert that to UCS-2 for return to the caller
        *ppuszString = (UniChar *) calloc( iLen, sizeof( UniChar ));
        if ( *ppuszString ) {
            StrConvert( (PCH)pszEUC, (PCH)(*ppuszString), uconvEUC, NULL );
            bResult = CONV_OK;
        }
        free( pszEUC );
    }

done:
    free( kanji );
    return bResult;
}


/*
INT _Optlink PreparePhrases( PVOID pSession )
{
    // Split into phrases with jl_tan_conv()
    // Return jl_bun_suu()
}


INT _Optlink PrepareCandidates( PVOID pSession )
{
    // Add candidates to buffer with jl_zenkouhou()
    // Return jl_zenkouho_suu()
}


INT _Optlink SetCandidate( PVOID pSession )
{
    // Move to the next/previous candidate with jl_next()/jl_previous()
}
*/
