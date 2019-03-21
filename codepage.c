#define INCL_WINATOM
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unidef.h>
#include <uconv.h>

#include "wnnhook.h"
#include "codepage.h"


/* ------------------------------------------------------------------------- *
 * IsDBCSLeadByte                                                            *
 *                                                                           *
 * Determine if the given byte is the leading-byte of a multi-byte DBCS      *
 * character.  Based on code by Alessandro Cantatore.                        *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   CHAR  ch   : The byte value to query.                                   *
 *   PBYTE pDBCS: The leading byte-ranges for the specified codepage.        *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 * ------------------------------------------------------------------------- */
BOOL _Optlink IsDBCSLeadByte( CHAR ch, PBYTE pDBCS )
{
   while ( *pDBCS )
      if (( ch >= *pDBCS++ ) && ( ch <= *pDBCS++ )) return TRUE;
   return FALSE;
}


/* ------------------------------------------------------------------------- *
 * GetEucCodepage                                                            *
 *                                                                           *
 * Return the OS/2 codepage number for Extended Unix Code encoding that      *
 * corresponds to the specified language.                                    *
 *                                                                           *
 * RETURNS: USHORT                                                           *
 * The codepage number.                                                      *
 * ------------------------------------------------------------------------- */
USHORT _Optlink GetEucCodepage( USHORT usLang )
{
    switch ( usLang ) {
        case MODE_JP: return 954;
        case MODE_KR: return 970;
        case MODE_CN: return 1383;
        case MODE_TW: return 964;
        default:      return 0;
    }
}


/* ------------------------------------------------------------------------- *
 * CreateUconvObject                                                         *
 *                                                                           *
 * Create a conversion object for the specified codepage to/from UCS-2.      *
 * The caller will be responsible for freeing it with UniFreeUconvObject()   *
 * once it is no longer needed.                                              *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   ULONG        ulCP : The codepage number.                            (I) *
 *   UconvObject *uconv: Pointer to receive the created UconvObject.     (O) *
 *                                                                           *
 * RETURNS: ULONG                                                            *
 *   ULS API return code                                                     *
 * ------------------------------------------------------------------------- */
ULONG _Optlink CreateUconvObject( ULONG ulCP, UconvObject *uconv )
{
    UniChar suCP[ 32 ];         // conversion specifier (UCS-2 string)
    ULONG   rc = ULS_SUCCESS;

    if ( ulCP == 0 )
        suCP[ 0 ] = L'@';
    else
        rc = UniMapCpToUcsCp( ulCP, suCP, 12 );

    if ( rc == ULS_SUCCESS ) {
        UniStrcat( suCP, (UniChar *) L"@map=display,path=no");
        rc = UniCreateUconvObject( suCP, uconv );
    }
    return rc;
}


/* ------------------------------------------------------------------------- *
 * StrConvert                                                                *
 *                                                                           *
 * Converts the input string from one codepage to another.                   *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   PSZ         pszInput:  Input string to be converted.                    *
 *   PSZ         pszOutput: Output buffer (must be at least 4*input length). *
 *   UconvObject uconvFrom: Conversion object for the source codepage.       *
 *   UconvObject uconvTo:   Conversion object for the target codepage.       *
 *                                                                           *
 * RETURNS: PSZ                                                              *
 *   Pointer to the output buffer.                                           *
 * ------------------------------------------------------------------------- */
PSZ _Optlink StrConvert( PSZ pszInput, PSZ pszOutput, UconvObject uconvFrom, UconvObject uconvTo )
{
    UniChar *psu;               // UCS-2 conversion buffer
    ULONG   in_len,
            out_len,
            rc;

    if ( !uconvFrom || !uconvTo ) return pszOutput;

    in_len = 2 * strlen( pszInput );
    psu = (UniChar *) calloc( in_len + 1, sizeof( UniChar ));
    if ( psu == NULL )
        return pszOutput;

    out_len = 4 * strlen( pszInput );
    rc = UniStrToUcs( uconvFrom, psu, pszInput, in_len + 1 );
    if ( rc == ULS_SUCCESS ) {
        rc = UniStrFromUcs( uconvTo, pszOutput, psu, out_len + 1 );
    }
    free( psu );
    return pszOutput;
}



