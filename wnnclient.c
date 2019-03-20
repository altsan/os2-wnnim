#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wnn/jllib.h>
#include <wnn/rk_spclval.h>

#include "codepage.h"
#include "wnnhook.h"
#include "wnnim.h"

// Romkan API functions exported by WNN library.  (There are a few others which
// we don't need, like romkan_next() and romkan_dispmode().)
//

extern void   _cdecl romkan_set_lang( char * );
extern int    _cdecl romkan_init( char*, letter, char, letter (* _cdecl keyinfn)(),int (* _cdecl bytcntfn)() );
extern letter _cdecl romkan_getc( void );
extern letter _cdecl romkan_ungetc( letter);
extern void   _cdecl romkan_clear( void );


// ============================================================================
// CALLBACK FUNCTIONS
// ============================================================================

/* ------------------------------------------------------------------------- *
 * NextCharacter                                                             *
 *                                                                           *
 * Callback function registered with romkan_init(): returns the next input   *
 * character for conversion (romkan_getc() and romkan_next() will use it).   *
 * ------------------------------------------------------------------------- */
letter _cdecl NextCharacter()
{
    letter ltr = (letter) \
                    ((( global.usCharIdx < MAX_CHAR_BUF ) &&
                       global.szRomaji[ global.usCharIdx ]  ) ?
                        global.szRomaji[ global.usCharIdx++ ] : EOLTTR );
    return ltr;
}


/* ------------------------------------------------------------------------- *
 * CharacterByteCount                                                        *
 *                                                                           *
 * Callback function registered with romkan_init(): returns whether an       *
 * input character value is a single- or double-byte character for the       *
 * active input codepage.                                                    *
 * ------------------------------------------------------------------------- */
int * _cdecl CharacterByteCount( char *pChar )
{
    // TODO query the current OS/2 codepage
    return 1;       // temp
}


/* ------------------------------------------------------------------------- *
 * ErrorFunc                                                                 *
 *                                                                           *
 * Callback function registered with jl_open_lang(): used to output error    *
 * messages.  Since we don't use this feature, it simply returns 1.          *
 * ------------------------------------------------------------------------- */
int _cdecl ErrorFunc( const char *pcsz )
{
    //fprintf( stderr, "%s\n", pcsz );
    return ( 1 );
}



// ============================================================================
// OTHER FUNCTIONS
// ============================================================================

/* ------------------------------------------------------------------------- *
 * InitConversionMethod                                                      *
 * ------------------------------------------------------------------------- */
INT InitConversionMethod( USHORT usLang, PVOID *ppSession )
{
    // Pointers to handler functions
    PINT hnd_err( PCSZ ) = ErrorFunc;
    PINT hnd_msg( PCSZ ) = ErrorFunc;

    struct wnn_env *wnnenv = NULL;  // Wnn environment object
    struct wnn_buf *bdata = NULL;   // Wnn session buffer

    PSZ     pszEnv,                 // Return pointer for getenv()
            pszServer,              // Host address of jserver
            pszUser;                // User/environment name to use on the server
    CHAR    fzk[ 1024 ];
    INT     result = 0;

    // Get the server and user names to use.
    pszEnv = getenv("JSERVER");
    pszServer = strdup( pszEnv? pszEnv: "localhost");
    pszEnv = getenv("USER");
    pszUser = strdup( pszEnv? pszEnv: "root");

    // Connect to the server.
    bdata = jl_open_lang( pszUser, pszServer, "ja_JP", NULL, hnd_err, hnd_msg, 0 );
    if (( bdata == NULL ) || ( jl_isconnect( bdata ) == 0 )) {
        printf("Failed to establish session with server %s.\n", pszServer );
        result = 1;
        goto done_connect;
    }

    if ( jl_fuzokugo_get( bdata, fzk ) == -1 ) {
        //  Environment isn't active on server, so initialize it now.
        wnnenv = jl_env_get( bdata );
        jl_set_env_wnnrc( wnnenv, "wnnenvrc", (void *)WNN_CREATE, NULL );
    }

    *ppSession = bdata;

done_connect:
    free( pszServer );
    free( pszUser );
    return 0;
}


/* ------------------------------------------------------------------------- *
 * InitInputMethod                                                           *
 * ------------------------------------------------------------------------- */
INT InitInputMethod( PSZ pszPath, PSZ pszLang )
{
    romkan_set_lang( pszLang );
    return ( romkan_init( pszPath, 0x08, 0, *NextCharacter, *CharacterByteCount ));
}


/* ------------------------------------------------------------------------- *
 * ConvertPhonetic                                                           *
 *                                                                           *
 * Convert the input ('romaji') buffer into phonetic characters ('kana') for *
 * the current language.  (We use the term 'kana', which is Japanese, but    *
 * this also applies to Korean Hangul.)                                      *
 *                                                                           *
 * Note that the converted result may consist of more than one actual        *
 * character value.                                                          *
 *                                                                           *
 * ------------------------------------------------------------------------- */
USHORT ConvertPhonetic( void )
{
    USHORT i,
           result;
    letter ltr;

    i = 0;
    result = KANA_INVALID;
    do {
        ltr = romkan_getc();
        if ( ltr == LTREOF )
            break;           // end of input, exit with the last status result
        else if ( ltr && is_HON( ltr )) {
            global.szKana[ i++ ] = (UCHAR)(ltr & 0xFF);
            result = KANA_COMPLETE;
        }
        else if ( ltr & 0x80000000 )
            result = KANA_PENDING;
        else // if ( ltr == NISEBP )
            break;          // error, exit with the last status result
        // TODO not sure how to identify KANA_CANDIDATE yet (only used for Korean)
    } while ( ltr );

    return result;
}

