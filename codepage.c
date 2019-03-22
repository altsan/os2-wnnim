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
 *   PCH         pchInput:  Input string to be converted.                    *
 *   PCH         pchOutput: Output buffer (must be at least 4*input length). *
 *   UconvObject uconvFrom: Conversion object for the source codepage.       *
 *                          If NULL, convert from UCS-2.                     *
 *   UconvObject uconvTo:   Conversion object for the target codepage.       *
 *                          If NULL, convert to UCS-2.                       *
 *                                                                           *
 * RETURNS: PCH                                                              *
 *   Pointer to the output buffer.                                           *
 * ------------------------------------------------------------------------- */
PCH _Optlink StrConvert( PCH pchInput, PCH pchOutput, UconvObject uconvFrom, UconvObject uconvTo )
{
    UniChar *psu;               // UCS-2 conversion buffer
    ULONG   in_len,
            out_len,
            rc;

    if ( !uconvFrom && !uconvTo )       // no conversion objects --> no conversion!
        return pchOutput;

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
    return pchOutput;
}



