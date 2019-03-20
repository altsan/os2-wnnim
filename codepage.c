#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unidef.h>
#include <uconv.h>

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
BOOL IsDBCSLeadByte( CHAR ch, PBYTE pDBCS )
{
   while ( *pDBCS )
      if (( ch >= *pDBCS++ ) && ( ch <= *pDBCS++ )) return TRUE;
   return FALSE;
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
 * ------------------------------------------------------------------------- *
ULONG CreateUconvObject( ULONG ulCP, UconvOject *uconv )
{
    UniChar     suCP[ 32 ];         // conversion specifier (UCS-2 string)
    ULONG       rc = ULS_SUCCESS;

    if ( ulCP == 0 )
        suCP[ 0 ] = L'@';
    else
        rc = UniMapCpToUcsCp( ulCP, suCP, 12 )
    if ( rc != ULS_SUCCESS )
        return rc;

    UniStrcat( suCP, (UniChar *) L"@map=display,path=no");
    rc = UniCreateUconvObject( suCP, uconv );
    if ( rc != ULS_SUCCESS )
        return rc;
}


/*
char *strconvert( char *input, unsigned long fromCP, unsigned long toCP )
{
    UconvObject uconvFrom,
                uconvTo;
    UniChar suCP[ 32 ];         // conversion specifier (UCS-2 string)
    UniChar *psu;               // UCS-2 conversion buffer
    unsigned long in_len,
                  out_len,
                  rc;
    char *output = NULL;

    // Create conversion objects for each codepage
    if ( fromCP == 0 ) suCP[ 0 ] = L'@';
    else if ( UniMapCpToUcsCp( fromCP, suCP, 12 ) != ULS_SUCCESS ) {
        printf("UniMapCpToUcsCp: %X\n", rc );
        return NULL;
    }
    UniStrcat( suCP, (UniChar *) L"@map=display,path=no");
    if ( UniCreateUconvObject( suCP, &uconvFrom ) != ULS_SUCCESS ) {
        printf("UniCreateUconvObject: %X\n", rc );
        return NULL;
    }

    if ( toCP == 0 ) suCP[ 0 ] = L'@';
    else if ( UniMapCpToUcsCp( toCP, suCP, 12 ) != ULS_SUCCESS ) {
        printf("UniMapCpToUcsCp: %X\n", rc );
        goto done2;
    }
    UniStrcat( suCP, (UniChar *) L"@map=display,path=no");
    if ( UniCreateUconvObject( suCP, &uconvTo ) != ULS_SUCCESS ) {
        printf("UniCreateUconvObject: %X\n", rc );
        goto done2;
    }

    in_len = strlen( input );
    psu = (UniChar *) calloc( in_len+1, sizeof(UniChar) );
    if ( psu == NULL ) {
        printf("Out of memory!\n");
        goto done1;
    }

    // Convert the string
    out_len = in_len * 4;
    rc = UniStrToUcs( uconvFrom, psu, input, in_len+1 );
    if (( rc != ULS_SUCCESS ) ||
        (( output = (char *) malloc( out_len+1 )) == NULL ))
    {
        printf("UniStrToUcs: %X\n", rc );
        free( psu );
        goto done1;
    }
    rc = UniStrFromUcs( uconvTo, output, psu, out_len+1 );
    if ( rc != ULS_SUCCESS ) {
        printf("UniStrFromUcs: %X\n", rc );
        free( output );
        output = NULL;
    }

    // Clean up and return
    free( psu );

done1:
    UniFreeUconvObject( uconvTo );
done2:
    UniFreeUconvObject( uconvFrom );
    return output;
}

*/
