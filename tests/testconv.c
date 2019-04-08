/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define INCL_DOSERRORS
#define INCL_WINATOM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <uconv.h>

#include "codepage.h"
#include "wnnhook.h"
#include "wnnim.h"
#include "wnnclient.h"


#define TEST_STRING     "ÇÌÇΩÇµÇÃÇ»Ç‹Ç¶ÇÕÇ»Ç©ÇÃÇ≈Ç∑ÅB"


IMCLIENTDATA global = {0};


int main( int argc, char *argv[] )
{
    UconvObject uconvSJIS;
    UniChar    *puszInput,
               *puszCandidate;
    struct wnn_buf *pSession = NULL;
    PSZ   arg,
          pszInput = NULL,
          pszOutput = NULL;
    INT   iCount,
          iLen,
          i;
    BOOL  fReading;
    ULONG rc;

    printf("Starting clause conversion tests.\n");

    rc = UniCreateUconvObject( (UniChar *) L"IBM-932@map=display,path=no", &uconvSJIS );
    if ( rc != ULS_SUCCESS ) {
        printf("UniCreateUconvObject error: 0x%X\n", rc );
        return 1;
    }

    rc = InitConversionMethod( NULL, MODE_JP, (PVOID *)&pSession );
    if ( rc == CONV_CONNECT ) {
        printf("Failed to establish session with FreeWnn server: %s.\n", global.szEngineError );
        return 1;
    }

    // Get the input string.
    for ( i = 1; i < argc; i++ ) {
        arg = argv[ i ];
        if ( *arg == '/' || *arg == '-') {
            arg++;
            if ( !(*arg) ) continue;
            if (( *arg == 'R') || ( *arg == 'r'))
                fReading = TRUE;
        }
        else pszInput = strdup( arg );
    }
    if ( !pszInput ) pszInput = strdup( TEST_STRING );

    puszInput = (UniChar *) calloc( strlen( pszInput ) + 1, sizeof( UniChar ));
    if ( !puszInput ) {
        printf("Error allocating buffer.\n");
        rc = 1;
        goto done;
    }

    rc = StrConvert( pszInput, (PCH) puszInput, uconvSJIS, NULL );
    if ( rc != ULS_SUCCESS ) {
        printf("ULS conversion error: 0x%X\n", rc );
        goto done;
    }

    rc = ConvertClause( pSession, puszInput );
    free( puszInput );
    if ( rc != CONV_OK ) {
        printf("\nError converting text: %s\n", global.szEngineError );
        goto done;
    }

    iCount = GetPhraseCount( pSession );
    if ( iCount == CONV_CONNECT ) {
        printf("\nError converting text: %s\n", global.szEngineError );
        goto done;
    }

    printf("\n%d phrases found.\n", iCount );
    for ( i = 0; i < iCount; i++ ) {
        UniChar *puszPhrase;
        printf("%c) ", i+65 );
        if ( CONV_OK == GetConvertedString( pSession, i, i+1, TRUE, &puszPhrase )) {
            iLen = UniStrlen( puszPhrase );
            pszOutput = (PSZ) malloc( iLen + 1 );
            StrConvert( (PCH) puszPhrase, pszOutput, NULL, uconvSJIS );
            printf("%s\n", pszOutput );
            free( puszPhrase );
            free( pszOutput );
        }
    }
    printf("\n");

    if ( CONV_OK == GetConvertedString( pSession, 0, -1, FALSE, &puszCandidate )) {
        printf("Default conversion:\n");
        iLen = UniStrlen( puszCandidate );
        pszOutput = (PSZ) malloc( iLen + 1 );
        StrConvert( (PCH) puszCandidate, pszOutput, NULL, uconvSJIS );
        printf("%s\n", pszOutput );
        free( pszOutput );
        free( puszCandidate );
    }

    iCount = PrepareCandidates( pSession );
    if ( iCount == CONV_CONNECT ) {
        printf("Lost server connection.\n");
        rc = 1;
        goto done;
    }
    printf("\nAll candidates (%d)\n", iCount );
    for ( i = 0; i < iCount; i++ ) {
        SetCandidate( pSession, TRUE );
        if ( CONV_OK == GetConvertedString( pSession, 0, -1, FALSE, &puszCandidate )) {
            iLen = UniStrlen( puszCandidate );
            pszOutput = (PSZ) malloc( iLen + 1 );
            StrConvert( (PCH) puszCandidate, pszOutput, NULL, uconvSJIS );
            printf("%d) %s\n", i+1, pszOutput );
            free( pszOutput );
            free( puszCandidate );
        }
    }

done:
    FinishConversionMethod( pSession );
    free( pszInput );

    return rc;
}


