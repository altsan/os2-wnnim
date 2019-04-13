/****************************************************************************
 * clipfuncs.c                                                              *
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

#define INCL_WIN
#define INCL_PM
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uconv.h>

//#include "wnnhook.h"
//#include "wnnim.h"
#include "clipfuncs.h"


BOOL ClipPutUniText( HAB hab, UniChar *puszBuffer, ATOM cfUnicode )
{
    UniChar *puszShareMem;
    ULONG    usLen;
    BOOL     fRC = FALSE;

    // Place the UCS-2 string on the clipboard as "text/unicode"
    usLen = UniStrlen( puszBuffer );
    if ( NO_ERROR == DosAllocSharedMem( (PVOID) &puszShareMem, NULL,
                                        (usLen+1) * sizeof( UniChar ),
                                        fALLOCSHR ))
    {
        memset( puszShareMem, 0, (usLen+1) * sizeof( UniChar ));
        UniStrncpy( puszShareMem, puszBuffer, usLen );
        if ( WinSetClipbrdData( hab, (ULONG) puszShareMem,
                                cfUnicode, CFI_POINTER ))
            fRC = TRUE;
    }
    return fRC;
}


BOOL ClipPutText( HAB hab, PSZ pszBuffer, ULONG ulFmt )
{
    PSZ     pszShareMem;
    ULONG   usLen;
    BOOL    fRC = FALSE;

    usLen = strlen( pszBuffer );
    if ( NO_ERROR == DosAllocSharedMem( (PVOID) &pszShareMem, NULL,
                                        usLen + 1, fALLOCSHR ))
    {
        memset( pszShareMem, 0, usLen + 1 );
        strncpy( pszShareMem, pszBuffer, usLen );
        if ( WinSetClipbrdData( hab, (ULONG) pszShareMem,
                                ulFmt, CFI_POINTER ))
            fRC = TRUE;
    }
    return fRC;
}


/* ------------------------------------------------------------------------- *
 * CopyBitmap                                                                *
 * (From Charles Petzold, (c)1989-1994)                                      *
 * ------------------------------------------------------------------------- */
HBITMAP CopyBitmap (HAB hab, HBITMAP hbmSrc)
{
    BITMAPINFOHEADER2 bmp ;
    HBITMAP          hbmDst ;
    HDC              hdcSrc, hdcDst ;
    HPS              hpsSrc, hpsDst ;
    POINTL           aptl[3] ;
    SIZEL            sizl ;

                                  // Create memory DC's and PS's

    hdcSrc = DevOpenDC (hab, OD_MEMORY, "*", 0L, NULL, NULLHANDLE) ;
    hdcDst = DevOpenDC (hab, OD_MEMORY, "*", 0L, NULL, NULLHANDLE) ;

    sizl.cx = sizl.cy = 0 ;
    hpsSrc = GpiCreatePS (hab, hdcSrc, &sizl, PU_PELS    | GPIF_DEFAULT |
                                                     GPIT_MICRO | GPIA_ASSOC) ;

    hpsDst = GpiCreatePS (hab, hdcDst, &sizl, PU_PELS    | GPIF_DEFAULT |
                                                     GPIT_MICRO | GPIA_ASSOC) ;

                                  // Create bitmap
    bmp.cbFix = sizeof (BITMAPINFOHEADER2) ;
    GpiQueryBitmapInfoHeader (hbmSrc, &bmp) ;
    hbmDst = GpiCreateBitmap (hpsDst, &bmp, 0L, NULL, NULL) ;

                                  // Copy from source to destination

    if (hbmDst != NULLHANDLE)
         {
         GpiSetBitmap (hpsSrc, hbmSrc) ;
         GpiSetBitmap (hpsDst, hbmDst) ;

         aptl[0].x = aptl[0].y = 0 ;
         aptl[1].x = bmp.cx ;
         aptl[1].y = bmp.cy ;
         aptl[2]   = aptl[0] ;

         GpiBitBlt (hpsDst, hpsSrc, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE) ;
         }
                                  // Clean up
    GpiDestroyPS (hpsSrc) ;
    GpiDestroyPS (hpsDst) ;
    DevCloseDC (hdcSrc) ;
    DevCloseDC (hdcDst) ;

    return hbmDst ;
}



