/****************************************************************************
 * codepage.c                                                               *
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
#define INCL_WINATOM
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unidef.h>
#include <uconv.h>

#include <PMPRINTF.H>

#include "wnnhook.h"
#include "codepage.h"


// Fullwidth codepoints
#define CHOUON  (0x30FC)        // long vowel modifier
#define DAKUTN  (0x309B)        // voiced modifier
#define HNDAKU  (0x309C)        // semivoiced modifier
#define MNMARU  (0x3002)        // CJK period
#define HRKKAG  (0x300C)        // CJK left quote
#define TJIKAG  (0x300D)        // CJK right quote
#define TOUTEN  (0x3001)        // CJK comma
#define NKPOTU  (0x30FB)        // middot

// Halfwidth codepoints
#define HKCHOU  (0xFF70)
#define HKDKTN  (0xFF9E)
#define HKHNDK  (0xFF9F)
#define HKMARU  (0xFF61)
#define HKHRKG  (0xFF62)
#define HKTJKG  (0xFF63)
#define HKTTEN  (0xFF64)
#define HKNKPT  (0xFF65)

#define HIRBGN  (0x3041)        // first hiragana character in UCS-2
#define HIREND  (0x3093)        // last hiragana character in UCS-2
#define KATBGN  (0x30A1)        // first katakana character in UCS-2
#define KATEND  (0x30F6)        // last katakana character in UCS-2

#define is_hira( l )    ( HIRBGN <= (l) && (l) <= HIREND )
#define is_kata( l )    ( KATBGN <= (l) && (l) <= KATEND )


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
BOOL _System IsDBCSLeadByte( CHAR ch, PBYTE pDBCS )
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
USHORT _System GetEucCodepage( USHORT usLang )
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
ULONG _System CreateUconvObject( ULONG ulCP, UconvObject *uconv )
{
    UniChar suCP[ 32 ];         // conversion specifier (UCS-2 string)
    ULONG   rc = ULS_SUCCESS;

    if ( ulCP == 0 )
        suCP[ 0 ] = L'@';
    else
        rc = UniMapCpToUcsCp( ulCP, suCP, 12 );

    if ( rc == ULS_SUCCESS ) {
        UniStrcat( suCP, (UniChar *) L"@map=display,path=yes");
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
 *   PCH         pchInput:  Input string to be converted.                    *
 *   PCH         pchOutput: Output buffer (if converting to UCS-2, must be   *
 *                          at least 2x the length of pchInput; otherwise,   *
 *                          must be at least 4x the length of pszInput).     *
 *   UconvObject uconvFrom: Conversion object for the source codepage.       *
 *                          If NULL, convert from UCS-2.                     *
 *   UconvObject uconvTo:   Conversion object for the target codepage.       *
 *                          If NULL, convert to UCS-2.                       *
 *                                                                           *
 * RETURNS: ULONG                                                            *
 *   ULS return code.                                                        *
 * ------------------------------------------------------------------------- */
ULONG _System StrConvert( PCH pchInput, PCH pchOutput, UconvObject uconvFrom, UconvObject uconvTo )
{
    UniChar *psu;               // UCS-2 conversion buffer
    ULONG   in_len,
            out_len,
            rc;

    if ( !uconvFrom && !uconvTo )       // no conversion objects --> no conversion!
        return 0;

    in_len  = 2 * strlen( pchInput );   // allow up to double the input string length

    if ( uconvFrom ) {
        // Convert input string to UCS-2
        psu = (UniChar *) calloc( in_len + 1, sizeof( UniChar ));
        if ( psu == NULL )
            goto done;
        rc = UniStrToUcs( uconvFrom, psu, pchInput, in_len + 1 );
    }
    else {
        // Assume we were given a UCS-2 string
        psu = (UniChar *) pchInput;
        rc = ULS_SUCCESS;
    }

    if ( !uconvTo )
        UniStrcpy( (UniChar *)pchOutput, psu );
    else if ( rc == ULS_SUCCESS ) {
        // Convert UCS-2 string to output codepage
        out_len = 4 * UniStrlen( psu );     // allow up to 4x the UCS-2 string length
        rc = UniStrFromUcs( uconvTo, pchOutput, psu, out_len + 1 );
    }

    if ( uconvFrom ) free( psu );

done:
    return rc;
}


/* ------------------------------------------------------------------------- *
 * ConvertFullWidth                                                          *
 *                                                                           *
 * Convert all ASCII chars in the input string to UCS-2 fullwidth forms.     *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   PSZ      pszInput  : Input string (MBCS encoded)                        *
 *   UniChar *puczOutput: UCS-2 output buffer (already allocated)            *
 *   USHORT   usMax     : Size of output buffer                              *
 *                                                                           *
 * RETURNS: USHORT                                                           *
 *   Number of characters converted.                                         *
 * ------------------------------------------------------------------------- */
USHORT _System ConvertFullWidth( PSZ pszInput, UniChar *puczOutput, USHORT usMax )
{
    USHORT  i, j,
            usInLen;
    CHAR    c;
    UniChar uc;

    usInLen = strlen( pszInput );
    j = 0;
    for ( i = 0; ( j < usMax ) && ( i < usInLen ); i++ ) {
        c = pszInput[ i ];
//        _PmpfF(("Converting %c (%02X) to fullwidth form.", c, c ));
        if ( ! (( c > 0x20 ) && ( c < 0x7F ))) continue;
        if ( c == 0x7E )
            uc = 0x301C;        // convert tilde to wavy dash instead of U+FF5F
        else
            uc = (UniChar)( c + 0xFEE0 );
//        _PmpfF(("--> U+%04X", uc ));
        puczOutput[ j++ ] = uc;
    }
    puczOutput[ j ] = 0;

    return j;
}


/* ------------------------------------------------------------------------- *
 * ConvertHankaku                                                            *
 *                                                                           *
 * Convert all kana in the input string to halfwidth ('hankaku') katakana.   *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   UniChar *puczInput : Input string (UCS-2 encoded)                       *
 *   UniChar *puczOutput: UCS-2 output buffer (already allocated)            *
 *   USHORT   usMax     : Size of output buffer                              *
 *                                                                           *
 * RETURNS: USHORT                                                           *
 *   Number of characters converted.                                         *
 * ------------------------------------------------------------------------- */
USHORT _System ConvertHankaku( UniChar *puczInput, UniChar *puczOutput, USHORT usMax )
{
    // Lookup table of halfwidth katakana characters in UCS-2 encoding
    static UniChar ucHankata[][3] = {
        {0xFF67,0x0000,0}, {0xFF71,0x0000,0}, {0xFF68,0x0000,0}, {0xFF72,0x0000,0}, {0xFF69,0x0000,0}, {0xFF73,0x0000,0}, {0xFF6A,0x0000,0}, {0xFF74,0x0000,0}, {0xFF6B,0x0000,0}, {0xFF75,0x0000,0},
        {0xFF76,0x0000,0}, {0xFF76,0xFF9E,0}, {0xFF77,0x0000,0}, {0xFF77,0xFF9E,0}, {0xFF78,0x0000,0}, {0xFF78,0xFF9E,0}, {0xFF79,0x0000,0}, {0xFF79,0xFF9E,0}, {0xFF7A,0x0000,0}, {0xFF7A,0xFF9E,0},
        {0xFF7B,0x0000,0}, {0xFF7B,0xFF9E,0}, {0xFF7C,0x0000,0}, {0xFF7C,0xFF9E,0}, {0xFF7D,0x0000,0}, {0xFF7D,0xFF9E,0}, {0xFF7E,0x0000,0}, {0xFF7E,0xFF9E,0}, {0xFF7F,0x0000,0}, {0xFF7F,0xFF9E,0},
        {0xFF80,0x0000,0}, {0xFF80,0xFF9E,0}, {0xFF81,0x0000,0}, {0xFF81,0xFF9E,0}, {0xFF6F,0x0000,0}, {0xFF82,0x0000,0}, {0xFF82,0xFF9E,0}, {0xFF83,0x0000,0}, {0xFF83,0xFF9E,0}, {0xFF84,0x0000,0}, {0xFF84,0xFF9E,0},
        {0xFF85,0x0000,0}, {0xFF86,0x0000,0}, {0xFF87,0x0000,0}, {0xFF88,0x0000,0}, {0xFF89,0x0000,0},
        {0xFF8A,0x0000,0}, {0xFF8A,0xFF9E,0}, {0xFF8A,0xFF9F,0}, {0xFF8B,0x0000,0}, {0xFF8B,0xFF9E,0}, {0xFF8B,0xFF9F,0}, {0xFF8C,0x0000,0}, {0xFF8C,0xFF9E,0}, {0xFF8C,0xFF9F,0},
        {0xFF8D,0x0000,0}, {0xFF8D,0xFF9E,0}, {0xFF8D,0xFF9F,0}, {0xFF8E,0x0000,0}, {0xFF8E,0xFF9E,0}, {0xFF8E,0xFF9F,0},
        {0xFF8F,0x0000,0}, {0xFF90,0x0000,0}, {0xFF91,0x0000,0}, {0xFF92,0x0000,0}, {0xFF93,0x0000,0},
        {0xFF6C,0x0000,0}, {0xFF94,0x0000,0}, {0xFF6D,0x0000,0}, {0xFF95,0x0000,0}, {0xFF6E,0x0000,0}, {0xFF96,0x0000,0},
        {0xFF97,0x0000,0}, {0xFF98,0x0000,0}, {0xFF99,0x0000,0}, {0xFF9A,0x0000,0}, {0xFF9B,0x0000,0},
        {0x30EF,0x0000,0}, {0xFF9C,0x0000,0}, {0x30F0,0x0000,0}, {0x30F1,0x0000,0}, {0xFF66,0x0000,0}, {0xFF9D,0x0000,0},
        {0xFF73,0xFF9E,0}, {0x30F5,0x0000,0}, {0x30F6,0x0000,0}
    };

    UniChar in, *out, *p;
    USHORT  usCount;

    in = *puczInput;
    out = puczOutput;
    usCount = 0;
    while ( in && (( out - puczOutput) <= usMax )) {
        switch ( in ) {
            case CHOUON:
                *out++ = HKCHOU;
                usCount++;
                break;
            case DAKUTN:
                *out++ = HKDKTN;
                usCount++;
                break;
            case HNDAKU:
                *out++ = HKHNDK;
                usCount++;
                break;
            case MNMARU:
                *out++ = HKMARU;
                usCount++;
                break;
            case HRKKAG:
                *out++ = HKHRKG;
                usCount++;
                break;
            case TJIKAG:
                *out++ = HKTJKG;
                usCount++;
                break;
            case TOUTEN:
                *out++ = HKTTEN;
                usCount++;
                break;
            case NKPOTU:
                *out++ = HKNKPT;
                usCount++;
                break;

            default:
                if ( is_kata( in )) {
                    p = ucHankata[ in - KATBGN ];
                    UniStrcat( out, p );
                    out += UniStrlen( p );
                    usCount++;
                }
                else if ( is_hira( in )) {
                    p = ucHankata[ in - HIRBGN ];
                    UniStrcat( out, p );
                    out += UniStrlen( p );
                    usCount++;
                }
                else
                    *out++ = in;
        }
    }
    *out = 0;

    return usCount;
}


