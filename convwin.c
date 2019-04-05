/****************************************************************************
 * convwin.c                                                                *
 * Conversion overlay window.                                               *
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
#define INCL_DOSNLS
#define INCL_WIN
#define INCL_PM
#define INCL_GPI
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uconv.h>

#include "wnnhook.h"
#include "codepage.h"
#include "ids.h"
#include "wnnim.h"
#include "wnnclient.h"
#include "convwin.h"


#define FTYPE_NOTFOUND 0    // font does not exist
#define FTYPE_FOUND    1    // font name was found
#define FTYPE_LANGUAGE 2    // font supports requested charset
#define FTYPE_UNICODE  4    // font supports Unicode
#define FTYPE_BITMAP   8    // font is a bitmap font



MRESULT EXPENTRY CWinDisplayProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             DoPaint( HWND hwnd, HPS hps, PCWDATA pCtl );
BYTE             ResolveFont( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCell, USHORT flLang );
BOOL             SetFont( HWND hwnd, PCWDATA pCtl );
LONG             GetCurrentDPI( HWND hwnd );
void             UpdateWidth( HWND hwnd, HPS hps, PCWDATA pCtl );



/* ------------------------------------------------------------------------- *
 * CWinRegisterClass                                                         *
 *                                                                           *
 * Registers the conversion window class.                                    *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HAB hab: Application's anchor-block handle.                             *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 *   Return code from WinRegisterClass.                                      *
 * ------------------------------------------------------------------------- */
BOOL CWinRegisterClass( HAB hab )
{
    return WinRegisterClass( hab, WC_WNNIMCONVWIN, CWinDisplayProc,
                             CS_SIZEREDRAW, sizeof( PVOID ));
}


/* ------------------------------------------------------------------------- *
 * Window procedure for the clause conversion overlay window.                *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY CWinDisplayProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    PWNDPARAMS  pwp;            // pointer to window parameters
    PCWDATA     pCtl;           // pointer to private control data
    HPS         hps;
    RECTL       rcl;
    USHORT      usPhrase,
                usStart,
                usLen,
                usBuf,
                i;
    PUSHORT     pusArray;
    UniChar    *puszTemp;


    switch( msg ) {

        case WM_CREATE:
            // Initialize the private control data structure
            //   (using calloc automatically memsets it to 0)
            if ( !( pCtl = (PCWDATA) calloc( 1, sizeof( CWDATA ))))
                return (MRESULT) TRUE;
            WinSetWindowPtr( hwnd, 0, pCtl );

            // Get the control initialization data
            if ( mp1 ) {
                memcpy( &(pCtl->ctldata), mp1, ((PCWCTLDATA)mp1)->cb );
            }
            else {
                // Caller didn't give us anything, so set some sensible defaults
                pCtl->ctldata.cb = sizeof( CWCTLDATA );
                pCtl->ctldata.fsMode = 0;
            }
            pCtl->id = ((PCREATESTRUCT)mp2)->id;
            pCtl->lDPI = GetCurrentDPI( hwnd );
            pCtl->usCurrentPhrase = CWT_NONE;
            SetFont( hwnd, pCtl );

            // Save it to the window words
            WinSetWindowPtr( hwnd, 0, pCtl );
            return (MRESULT) FALSE;


        case WM_DESTROY:
            // Deallocate our private data
            if (( pCtl = WinQueryWindowPtr( hwnd, 0 )) != NULL ) {
                if ( pCtl->puszText ) free( pCtl->puszText );
                if ( pCtl->pusPhraseEnd ) free( pCtl->pusPhraseEnd );
                free( pCtl );
            }
            break;


        case WM_PAINT:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            hps = WinBeginPaint( hwnd, NULLHANDLE, NULLHANDLE );
            DoPaint( hwnd, hps, pCtl );
            WinEndPaint( hps );
            return (MRESULT) 0;


        case WM_PRESPARAMCHANGED:
            // Font or colours changed, so force a redraw
            switch ( (ULONG) mp1 ) {
                case PP_FONTNAMESIZE:
                    pCtl = WinQueryWindowPtr( hwnd, 0 );
                    SetFont( hwnd, pCtl );
                case PP_BACKGROUNDCOLOR:
                case PP_BACKGROUNDCOLORINDEX:
                case PP_FOREGROUNDCOLOR:
                case PP_FOREGROUNDCOLORINDEX:
                    WinInvalidateRect( hwnd, NULL, FALSE );
                    break;
                default: break;
            }
            break;


        case WM_SETFOCUS:
            if ( SHORT1FROMMP( mp2 )) {     // Got focus
                pCtl = WinQueryWindowPtr( hwnd, 0 );
                WinCreateCursor( hwnd, 0, 0,
                                 0, pCtl? pCtl->lCursorHeight : 0,
                                 CURSOR_SOLID | CURSOR_FLASH, NULL );
                WinShowCursor( hwnd, TRUE );
                WinInvalidateRect( hwnd, NULL, FALSE );
            }
            else                            // Lost focus
                WinDestroyCursor( hwnd );
            break;


        case WM_QUERYWINDOWPARAMS:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            pwp = (PWNDPARAMS) mp1;
            if ( !pwp ) return (MRESULT) FALSE;

            if ( pwp->fsStatus & WPM_CTLDATA ) {
                pwp->cbCtlData = sizeof( PCWCTLDATA );
                memcpy( pwp->pCtlData, &(pCtl->ctldata), pwp->cbCtlData );
                return (MRESULT) TRUE;
            }
            else if ( pwp->fsStatus & WPM_CBCTLDATA ) {
                pwp->cbCtlData = sizeof( PCWCTLDATA );
                return (MRESULT) TRUE;
            }
            return (MRESULT) FALSE;


        case WM_SETWINDOWPARAMS:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            pwp = (PWNDPARAMS) mp1;
            if ( !pwp ) return (MRESULT) 0;

            if (( pwp->fsStatus & WPM_CTLDATA )           &&
                ( pwp->cbCtlData == sizeof( PCWCTLDATA )) &&
                ( pwp->pCtlData != NULL )                   )
            {
                memcpy( &(pCtl->ctldata), pwp->pCtlData, pwp->cbCtlData );
            }
            return (MRESULT) 0;


        case WM_SIZE:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            WinQueryWindowRect( hwnd, &rcl );
            pCtl->lCursorHeight = rcl.yTop;
            break;


        // --------------------------------------------------------------------
        // Custom messages defined for this control
        //

        /* .................................................................. *
         * CWM_QUERYTEXTLENGTH                                                *
         * Get length of phrase or clause text, in UniChars (_not_ bytes).    *
         * If the caller plans to use this value to create a buffer for       *
         * CWM_GETTEXT, it should add 1 in order to allow for string          *
         * termination.                                                       *
         *  - mp1:                                                            *
         *     USHORT: Phrase number (from 0), or CWT_ALL for entire clause.  *
         *     USHORT: Unused, should be 0.                                   *
         *  - mp2:                                                            *
         *     Unused, should be 0.                                           *
         *  Returns USHORT:                                                   *
         *  Number of UCS-2 characters in requested string.                   *
         * .................................................................. */
        case CWM_QUERYTEXTLENGTH:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) 0;

            usLen = 0;
            usPhrase = SHORT1FROMMP( mp1 );
            if ( usPhrase == CWT_ALL )
                usLen = pCtl->usTextLen;
            else if ( usPhrase >= pCtl->usPhraseCount )
                usLen = 0;
            else if ( pCtl->pusPhraseEnd != NULL ) {
                usStart = usPhrase? 1 + pCtl->pusPhraseEnd[ usPhrase-1 ]: 0;
                usLen = pCtl->pusPhraseEnd[ usPhrase ] - usStart + 1;
            }
            return (MRESULT) usLen;


        /* .................................................................. *
         * CWM_GETTEXT                                                        *
         * Get phrase or clause text.                                         *
         *  - mp1:                                                            *
         *     USHORT: Phrase number (from 0), or CWT_ALL for entire clause.  *
         *     USHORT: Size of buffer pointed to by mp2                       *
         *  - mp2:                                                            *
         *     UniChar *: Pointer to UCS-2 buffer to receive string.          *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_GETTEXT:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;
            if ( !pCtl->puszText ) return (MRESULT) FALSE;

            usPhrase = SHORT1FROMMP( mp1 );
            if ( usPhrase == CWT_ALL ) {
                usStart = 0;
                usLen = min( SHORT2FROMMP( mp1 ), pCtl->usTextLen );
            }
            else if ( usPhrase >= pCtl->usPhraseCount )
                return (MRESULT) FALSE;     // invalid phrase number
            else if ( pCtl->pusPhraseEnd != NULL ) {
                usStart = usPhrase? 1 + pCtl->pusPhraseEnd[ usPhrase-1 ]: 0;
                usLen = min( SHORT2FROMMP( mp1 ),
                             pCtl->pusPhraseEnd[ usPhrase ] - usStart );
            }
            else
                return (MRESULT) FALSE;
            UniStrncpy( (UniChar *)mp2, pCtl->puszText + usStart, usLen );
            return (MRESULT) TRUE;


        /* .................................................................. *
         * CWM_SETTEXT                                                        *
         * Set phrase or clause text.  If setting a specific phrase, the      *
         * phrase boundaries will be updated automatically as needed.  If     *
         * setting the entire clause text, all phrase boundaries will be      *
         * cleared.                                                           *
         *  - mp1:                                                            *
         *     USHORT: Phrase number (from 0), or CWT_ALL for entire clause.  *
         *     USHORT: Length of string in UniChars (not including terminator)*
         *  - mp2:                                                            *
         *     UniChar *: Pointer to UCS-2 string, or NULL to clear text.     *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_SETTEXT:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;

            usLen = SHORT2FROMMP( mp1 );
            usPhrase = SHORT1FROMMP( mp1 );
            if ( usPhrase == CWT_ALL ) {
                // Replace entire text
                if ( !pCtl->puszText || (( usLen + 1 ) > pCtl->usBufLen )) {
                    usBuf = max( pCtl->usBufLen + BUFFER_INCREMENT, usLen + 1 );
                    puszTemp = (UniChar *) malloc( usBuf );
                    if ( !puszTemp ) return FALSE;
                    if ( pCtl->puszText ) free( pCtl->puszText );
                    pCtl->puszText = puszTemp;
                    pCtl->usBufLen = usBuf;
                }
                UniStrncpy( pCtl->puszText, (UniChar *)mp2, usLen );
                pCtl->usTextLen = usLen;
                if ( pCtl->pusPhraseEnd ) free( pCtl->pusPhraseEnd );
                pCtl->usPhraseCount = 0;
            }
            else if ( usPhrase >= pCtl->usPhraseCount )
                return (MRESULT) FALSE;     // invalid phrase number
            else if ( pCtl->puszText && pCtl->pusPhraseEnd ) {
                // Replace only the indicated phrase
                usBuf = pCtl->usTextLen + usLen + 1;
                if ( usBuf > pCtl->usBufLen )
                    usBuf = max( pCtl->usBufLen + BUFFER_INCREMENT, usBuf );
                puszTemp = (UniChar *) malloc( usBuf );
                if ( !puszTemp ) return (MRESULT) FALSE;

                // Add all phrases before the modified one to new buffer
                puszTemp[ 0 ] = 0;
                usStart = 0;
                for ( i = 0; i < usPhrase; i++ ) {
                    usLen = pCtl->pusPhraseEnd[ i ] - usStart + 1;
                    UniStrncat( puszTemp,
                                pCtl->puszText + usStart,
                                usLen );
                    usStart = pCtl->pusPhraseEnd[ i ] + 1;
                }
                // Move the start position past the phrase to be replaced
                usStart = pCtl->pusPhraseEnd[ usPhrase ] + 1;

                // Append the modified phrase and set its end position
                usLen = SHORT2FROMMP( mp1 );
                UniStrncat( puszTemp, (UniChar *) mp2, usLen );
                pCtl->pusPhraseEnd[ usPhrase ] = ( usPhrase > 0 )?
                                                    pCtl->pusPhraseEnd[ usPhrase-1 ] + usLen - 1:
                                                    usLen - 1;
                // Append all subsequent phrases and update their end positions
                for ( i = usPhrase + 1; i < pCtl->usPhraseCount; i++ ) {
                    usLen = pCtl->pusPhraseEnd[ i ] - usStart + 1;
                    UniStrncat( puszTemp,
                                pCtl->puszText + usStart,
                                usLen );
                    pCtl->pusPhraseEnd[ i ] = usStart + usLen - 1;
                    usStart += usLen;
                }
                free( pCtl->puszText );
                pCtl->puszText = puszTemp;
                pCtl->usTextLen = UniStrlen( puszTemp );
                pCtl->usBufLen = usBuf;
            }
            else
                return (MRESULT) FALSE;
            hps = WinGetPS( hwnd );
            UpdateWidth( hwnd, hps, pCtl );
            WinReleasePS( hps );
            return (MRESULT) TRUE;


        /* .................................................................. *
         * CWM_ADDCHAR                                                        *
         * Append character to clause.  If phrases are defined, the last      *
         * phrase will be automatically expanded to include the new character.*
         *  - mp1:                                                            *
         *  - mp2:                                                            *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_ADDCHAR:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;

            return (MRESULT) TRUE;


        /* .................................................................. *
         * CWM_DELCHAR                                                        *
         * Delete last character from clause (and from the last phrase, if    *
         * defined).                                                          *
         *  - mp1:                                                            *
         *  - mp2:                                                            *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_DELCHAR:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;

            return (MRESULT) TRUE;


        /* .................................................................. *
         * CWM_SETPHRASES                                                     *
         * Set the component phrase boundaries within the current text.  Any  *
         * previously-defined phrase boundaries are cleared.                  *
         * NOTE: The last phrase boundary must always be the last character   *
         * of the current string.  The processing of this message will ensure *
         * this if necessary.                                                 *
         *  - mp1:                                                            *
         *     USHORT:  Number of phrase boundaries.                          *
         *  - mp2:                                                            *
         *     PUSHORT: Array of USHORT character indices indicating the      *
         *              final character of each phrase.  These are offsets    *
         *              in puszText.                                          *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_SETPHRASES:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;

            // Create the new boundary array
            usPhrase = (USHORT) mp1;
            pusArray = (PUSHORT) calloc( usPhrase, sizeof( USHORT ));
            if ( !pusArray ) return (MRESULT) FALSE;
            if ( pCtl->pusPhraseEnd ) free( pCtl->pusPhraseEnd );
            pCtl->pusPhraseEnd = pusArray;
            pCtl->usPhraseCount = usPhrase;

            // Copy the new values
            pusArray = (PUSHORT)mp2;
            for ( i = 0; i < usPhrase; i++ ) {
                pCtl->pusPhraseEnd[ i ] = ( i + 1 == usPhrase )?
                                            pCtl->usTextLen - 1:
                                            pusArray[ i ];
            }
            return (MRESULT) TRUE;


        /* .................................................................. *
         * CWM_SELECTPHRASE                                                   *
         * Set the specified phrase active.  The active or selected phrase is *
         * used as the target for word-phrase conversion.  This allows        *
         * individual components within the clause to be changed by the user. *
         *  - mp1:                                                            *
         *     USHORT: One of:                                                *
         *               CWT_NEXT   Select the next phrase (or the first      *
         *                          phrase if none is currently selected)     *
         *               CWT_PREV   Select the previous phrase (or the last   *
         *                          phrase if none is currently selected)     *
         *               CWT_FIRST  Select the first phrase in the clause     *
         *               CWT_LAST   Select the last phrase in the clause      *
         *               CWT_NONE   Deselect all phrases                      *
         *  - mp2:                                                            *
         *     Unused, should be 0.                                           *
         *  Returns BOOL                                                      *
         * .................................................................. */
        case CWM_SELECTPHRASE:
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;
            usPhrase = (USHORT) mp1;
            switch ( usPhrase ) {
                default:
                case CWT_NONE:
                    pCtl->usCurrentPhrase = CWT_NONE;
                    break;
                case CWT_FIRST:
                    pCtl->usCurrentPhrase = 0;
                    break;
                case CWT_LAST:
                    pCtl->usCurrentPhrase = pCtl->usPhraseCount - 1;
                    break;
                case CWT_NEXT:
                    if ( pCtl->usCurrentPhrase == CWT_NONE )
                        pCtl->usCurrentPhrase = 0;
                    else if ( 1 + pCtl->usCurrentPhrase >= pCtl->usPhraseCount )
                        return (MRESULT) FALSE;     // no next phrase!
                    else
                        pCtl->usCurrentPhrase++;
                    break;
                case CWT_PREV:
                    if ( pCtl->usCurrentPhrase == CWT_NONE )
                        pCtl->usCurrentPhrase = pCtl->usPhraseCount - 1;
                    else if ( pCtl->usCurrentPhrase < 1 )
                        return (MRESULT) FALSE;     // no previous phrase!
                    else
                        pCtl->usCurrentPhrase--;
                    break;
            }

            return (MRESULT) TRUE;


        /* .................................................................. *
         *                                                                    *
         *  - mp1:                                                            *
         *  - mp2:                                                            *
         *  Returns                                                           *
         * .................................................................. *
        case :
            pCtl = WinQueryWindowPtr( hwnd, 0 );
            if ( !pCtl ) return (MRESULT) FALSE;

            return (MRESULT) TRUE;
        */

    }

    return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ));
}


/* ------------------------------------------------------------------------- *
 * DoPaint                                                                   *
 * ------------------------------------------------------------------------- */
void DoPaint( HWND hwnd, HPS hps, PCWDATA pCtl )
{
    FONTMETRICS fm;                     // current font metrics
    SIZEF       sfCell;                 // character cell size
    LONG        cb,                     // length of current text, in bytes
                lHeight, lWidth,        // dimensions of control rectangle (minus border)
                lCell;                  // desired character-cell height
    PCH         pchText;                // pointer to current output text
    POINTL      ptl,                    // current drawing position
                ptl2;
    POINTL      aptl[ TXTBOX_COUNT ];
    RECTL       rcl,
                rclPhrase;
    LONG        lClrBG,
                lClrFG;
    ULONG       ulID;
    double      dSizeAdjust;
    USHORT      usStart,
                i;
    APIRET      rc;


    if ( !pCtl ) return;

    // Get the current colours
    GpiCreateLogColorTable( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
    rc = WinQueryPresParam( hwnd, PP_FOREGROUNDCOLOR, PP_FOREGROUNDCOLORINDEX,
                            &ulID, sizeof( LONG ), &lClrFG, QPF_ID2COLORINDEX );
    if ( !rc ) lClrFG = SYSCLR_WINDOWTEXT;

    rc = WinQueryPresParam( hwnd, PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                            &ulID, sizeof( LONG ), &lClrBG, QPF_ID2COLORINDEX );
    if ( !rc ) lClrBG = SYSCLR_WINDOW;

    // Get the window area and available text height
    WinQueryWindowRect( hwnd, &rcl );
    lWidth  = rcl.xRight - rcl.xLeft;
    lHeight = rcl.yTop - rcl.yBottom;

    // Set the character cell size.
    lCell = lHeight - 2;

    // Set up the font face
    if (( GpiCreateLogFont( hps, NULL, 1L, &(pCtl->fattrs) )) == GPI_ERROR ) return;

    // Set the text size
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );

    // Adjust the cell height to take the maximum character bbox into account
    dSizeAdjust = (double)(fm.lEmHeight) / fm.lMaxBaselineExt;
    if ( dSizeAdjust > 0 ) {
        if ( dSizeAdjust > 1 ) dSizeAdjust = 1;
        else if ( dSizeAdjust < 0.75 ) dSizeAdjust = 0.75;
        lCell *= dSizeAdjust;
    }
    sfCell.cy = MAKEFIXED( lCell, 0 );
    sfCell.cx = sfCell.cy;
    GpiSetCharBox( hps, &sfCell );
    GpiSetCharSet( hps, 1L );

    // Get the updated font metrics for positioning calculations
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );
    GpiSetTextAlignment( hps, TA_LEFT, TA_BASE );

    // Paint the background
    WinFillRect( hps, &rcl, lClrBG );

    // Draw the text
    if ( pCtl->puszText ) {
        ptl.x = 1;
        ptl.y = fm.lMaxDescender;
        GpiMove( hps, &ptl );

        if (( pCtl->usCurrentPhrase != CWT_NONE ) &&
             pCtl->usPhraseCount && pCtl->pusPhraseEnd )
        {
            // Draw each phrase, inverting fg/bg for the current phrase
            for ( i = 0, usStart = 0; i < pCtl->usPhraseCount; i++ ) {
                pchText = (PCH) pCtl->puszText + ( usStart * 2 );
                cb = ( 1 + pCtl->pusPhraseEnd[ i ] - usStart ) * 2;
                if ( i == pCtl->usCurrentPhrase ) {
                    GpiSetColor( hps, lClrBG );
                    GpiSetBackColor( hps, lClrFG );
                }
                else {
                    GpiSetColor( hps, lClrFG );
                    GpiSetBackColor( hps, lClrBG );
                }
                GpiQueryTextBox( hps, cb, pchText, TXTBOX_COUNT, aptl );
                GpiQueryCurrentPosition( hps, &ptl );
                rclPhrase.xLeft = aptl[ TXTBOX_BOTTOMLEFT ].x + ptl.x;
                rclPhrase.xRight = aptl[ TXTBOX_TOPRIGHT ].x + ptl.x;
                rclPhrase.yBottom = 1; // aptl[ TXTBOX_BOTTOMLEFT ].y + ptl.y;
                rclPhrase.yTop = rcl.yTop - 2; // aptl[ TXTBOX_TOPRIGHT ].y + ptl.y;
                GpiCharStringPos( hps, &rclPhrase, CHS_CLIP | CHS_OPAQUE, cb, pchText, NULL );
                usStart = pCtl->pusPhraseEnd[ i ] + 1;
            }
        }
        else {
            // Draw the whole string at once
            pchText = (PCH) pCtl->puszText;
            cb = UniStrlen( pCtl->puszText ) * 2;
            GpiSetColor( hps, lClrFG );
            GpiSetBackColor( hps, lClrBG );
            GpiCharString( hps, cb, pchText );
        }
    }
    GpiQueryCurrentPosition( hps, &ptl2 );

    // Position the cursor
    WinCreateCursor( hwnd, ptl2.x, 0, 0, 0, CURSOR_SETPOS, &rcl );

#if 0
    // Paint a dashed border
    GpiSetLineType( hps, LINETYPE_LONGDASH );
    ptl.x = 0;
    ptl.y = 0;
    ptl2.x += fm.lEmInc - 1;
    ptl2.y = rcl.yTop - 1;
    GpiMove( hps, &ptl );
    GpiBox( hps, DRO_OUTLINE, &ptl2, 0, 0 );
#else
    // Underline the text
    ptl.x = 0;
    ptl.y = ptl2.y - (fm.lLowerCaseDescent / 2);
    ptl2.y = ptl.y;
    GpiMove( hps, &ptl );
    GpiLine( hps, &ptl2 );
#endif

}


/* ------------------------------------------------------------------------- *
 * ResolveFont                                                               *
 *                                                                           *
 * This function does a couple of things.  First, it looks to see if the     *
 * requested font name corresponds to a bitmap (a.k.a. raster) font.  If so, *
 * it sets the required attributes (in the passed FATTRS object) to indicate *
 * the largest point size which fits within the requested character-cell     *
 * size, unless said cell size is too small for any of the available point   *
 * sizes.  In the latter case, if an outline version of the font exists,     *
 * indicate to use that instead; otherwise, return the smallest bitmap point *
 * size that is available.                                                   *
 *                                                                           *
 * If no matching bitmap font can be found, look for an outline version of   *
 * the font.  If the requested font name does not exist at all, then simply  *
 * return FTYPE_NOTFOUND.                                                    *
 *                                                                           *
 * The function also checks to see if the font is reported as a Unicode font *
 * (i.e. one that supports the UNICODE glyphlist).                           *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HPS     hps        : Handle of the current presentation space.          *
 *   PSZ     pszFontFace: Face name of the font being requested.             *
 *   PFATTRS pfAttrs    : Address of a FATTRS object to receive the results. *
 *   LONG    lCell      : The requested character-cell size height           *
 *   USHORT  fsLang     : Requested language (charset) support flags         *
 *                        (if 0, Unicode is assumed to be requested)         *
 *                                                                           *
 * RETURNS: BYTE                                                             *
 *   One or more FTYPE_* flags                                               *
 * ------------------------------------------------------------------------- */
BYTE ResolveFont( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCell, USHORT fsLang )
{
    PFONTMETRICS pfm;               // array of FONTMETRICS objects
    LONG         i,                 // loop index
                 lSmIdx    = -1,    // index of smallest available font
                 lLargest  = 0,     // max baseline ext of largest font
                 cFonts    = 0,     // number of fonts found
                 cCount    = 0;     // number of fonts to return
    BOOL         bOutline  = FALSE, // does font include an outline version?
                 bLanguage = FALSE, // does font support requested language(s)?
                 bUnicode  = FALSE, // does font support Unicode?
                 bFound    = FALSE; // did we find a suitable bitmap font?
    BYTE         bResult   = 0;


    // Find the specific fonts which match the given face name
    cFonts = GpiQueryFonts( hps, QF_PUBLIC, pszFontFace,
                            &cCount, sizeof(FONTMETRICS), NULL );
    if ( cFonts < 1 ) return FTYPE_NOTFOUND;
    if (( pfm = (PFONTMETRICS) calloc( cFonts, sizeof(FONTMETRICS) )) == NULL )
        return FTYPE_NOTFOUND;
    GpiQueryFonts( hps, QF_PUBLIC, pszFontFace,
                   &cFonts, sizeof(FONTMETRICS), pfm );

    // Look for either an outline font, or the largest bitmap font that fits within the requested height
    for ( i = 0; !bOutline && ( i < cFonts ); i++ ) {
        bOutline  = FALSE;
        bLanguage = FALSE;
        bUnicode  = FALSE;
        if ( !( pfm[i].fsDefn & FM_DEFN_OUTLINE )) {
            if (( lSmIdx < 0 ) || ( pfm[i].lMaxBaselineExt < pfm[lSmIdx].lMaxBaselineExt ))
                lSmIdx = i;
            if (( pfm[i].lMaxBaselineExt <= lCell ) && ( pfm[i].lMaxBaselineExt > lLargest )) {
                lLargest = pfm[i].lMaxBaselineExt;
                bFound   = TRUE;
                strcpy( pfAttrs->szFacename, pfm[i].szFacename );
                pfAttrs->lMatch          = pfm[i].lMatch;
                pfAttrs->idRegistry      = pfm[i].idRegistry;
                pfAttrs->lMaxBaselineExt = pfm[i].lMaxBaselineExt;
                pfAttrs->lAveCharWidth   = pfm[i].lAveCharWidth;
                if ( pfm[i].fsSelection & fsLang ) bLanguage = TRUE;
                if ( pfm[i].fsType & FM_TYPE_UNICODE ) bUnicode = TRUE;
            }
        } else {
            bOutline = TRUE;
            strcpy( pfAttrs->szFacename, pfm[i].szFacename );
            pfAttrs->lMatch          = pfm[i].lMatch;
            pfAttrs->idRegistry      = pfm[i].idRegistry;
            pfAttrs->lMaxBaselineExt = pfm[i].lMaxBaselineExt;
            pfAttrs->lAveCharWidth   = pfm[i].lAveCharWidth;
            if ( pfm[i].fsSelection & fsLang ) bLanguage = TRUE;
            if ( pfm[i].fsType & FM_TYPE_UNICODE ) bUnicode = TRUE;
        }
    }

    // If nothing fits within the requested cell size, use the smallest available
    if ( !bFound && !bOutline && lSmIdx >= 0 ) {
        bFound = TRUE;
        strcpy( pfAttrs->szFacename, pfm[lSmIdx].szFacename );
        pfAttrs->lMatch          = pfm[lSmIdx].lMatch;
        pfAttrs->idRegistry      = pfm[lSmIdx].idRegistry;
        pfAttrs->lMaxBaselineExt = pfm[lSmIdx].lMaxBaselineExt;
        pfAttrs->lAveCharWidth   = pfm[lSmIdx].lAveCharWidth;
        if ( pfm[lSmIdx].fsSelection & fsLang ) bLanguage = TRUE;
        if ( pfm[lSmIdx].fsType & FM_TYPE_UNICODE ) bUnicode = TRUE;
    }
    free( pfm );

    bResult = FTYPE_FOUND;
    if ( !bOutline )            bResult |= FTYPE_BITMAP;
    if ( bLanguage || !fsLang ) bResult |= FTYPE_LANGUAGE;
    if ( bUnicode )             bResult |= FTYPE_UNICODE;

    return bResult;
}


/* ------------------------------------------------------------------------- *
 * SetFont                                                                   *
 * Update the current font attributes from the presentation parameters.      *
 * Note that instead of using the point size from the PP, we ignore it and   *
 * instead set the text size dynamically based on the client window height.  *
 * (This will not happen for bitmap fonts, unless this function is called    *
 * whenever the window is resized.)                                          *
 * ------------------------------------------------------------------------- */
BOOL SetFont( HWND hwnd, PCWDATA pCtl )
{
    BYTE        fbType;                 // font type
    RECTL       rcl;                    // client window dimensions
    USHORT      fsSel;                  // desired language selection flag
    LONG        lCell,                  // desired character-cell height
                lHeight, lWidth;        // dimensions of client window
    CHAR        szFont[ FACESIZE+4 ];   // current font pres.param
    PSZ         pszFontName;            // requested font family
    HPS         hps;
    APIRET      rc;


    if ( !pCtl ) return FALSE;

    if ( IS_LANGUAGE( MODE_JP, pCtl->ctldata.fsMode ))
        fsSel = FM_SEL_JAPAN;
    else if ( IS_LANGUAGE( MODE_CN, pCtl->ctldata.fsMode ))
        fsSel = FM_SEL_CHINA;
    else if ( IS_LANGUAGE( MODE_TW, pCtl->ctldata.fsMode ))
        fsSel = FM_SEL_TAIWAN;
    else if ( IS_LANGUAGE( MODE_KR, pCtl->ctldata.fsMode ))
        fsSel = FM_SEL_KOREA;
    else fsSel = 0;

    // Determine our basic dimensions
    WinQueryWindowRect( hwnd, &rcl );
    lWidth  = rcl.xRight - rcl.xLeft;
    lHeight = rcl.yTop - rcl.yBottom;
    lCell = lHeight - 2;

    // Set up the requested font and text encoding
    memset( &(pCtl->fattrs), 0, sizeof( FATTRS ));
    pCtl->fattrs.usRecordLength = sizeof( FATTRS );
    pCtl->fattrs.fsType         = FATTR_TYPE_MBCS;
    pCtl->fattrs.fsFontUse      = FATTR_FONTUSE_NOMIX;

    hps = WinGetPS( hwnd );
    fbType = 0;
    rc = WinQueryPresParam( hwnd, PP_FONTNAMESIZE, 0, NULL,
                            sizeof( szFont ), szFont, QPF_NOINHERIT );
    if ( rc ) {
        pszFontName = strchr( szFont, '.') + 1;
        fbType = ResolveFont( hps, pszFontName, &(pCtl->fattrs), lCell, fsSel );
        if ( fbType == FTYPE_NOTFOUND )
            rc = 0;
    }
    // Make sure we not only have a font, but that it supports Unicode (for the
    // output codepage) and the requested language (for the actual characters).
    if ( !rc || !( fbType & FTYPE_UNICODE ) || !( fbType & FTYPE_LANGUAGE )) {
        // Hmm, that font isn't appropriate.  Try and fall back to a good default.
        strcpy( szFont, "10.Times New Roman MT 30");
        pszFontName = strchr( szFont, '.') + 1;
        fbType = ResolveFont( hps, pszFontName, &(pCtl->fattrs), lCell, fsSel );
    }
    WinReleasePS( hps );

    if ( fbType & FTYPE_UNICODE )
        pCtl->fattrs.usCodePage = CP_UNICODE;
    else
        pCtl->fattrs.usCodePage = 0;

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * GetCurrentDPI                                                             *
 *                                                                           *
 * Queries the current vertical font resolution (a.k.a. DPI).                *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd: Window handle of control.                                    *
 *                                                                           *
 * RETURNS: LONG                                                             *
 *   Current screen DPI setting.                                             *
 * ------------------------------------------------------------------------- */
LONG GetCurrentDPI( HWND hwnd )
{
    HDC  hdc;           // device-context handle
    LONG lCap,          // value from DevQueryCaps
         lDPI;          // returned DPI value

    hdc = WinOpenWindowDC( hwnd );
    if ( DevQueryCaps( hdc, CAPS_VERTICAL_FONT_RES, 1, &lCap ))
        lDPI = lCap;
    if ( !lDPI )
        lDPI = 96;

    return lDPI;
}



/* ------------------------------------------------------------------------- *
 * UpdateWidth                                                               *
 * Update the window width based on the current text.                        *
 * ------------------------------------------------------------------------- */
void UpdateWidth( HWND hwnd, HPS hps, PCWDATA pCtl )
{
    HWND        hwndParent;
    FONTMETRICS fm;
    RECTL       rcl;
    POINTL      ptl;
    SIZEF       sfCell;                 // character cell size
    LONG        lHeight,
                lCell;
    ULONG       cb;
    PCH         pchText;
    double      dSizeAdjust;
    POINTL      aptl[ TXTBOX_COUNT ];


    if ( !pCtl ) return;

    hwndParent = WinQueryWindow( hwnd, QW_PARENT );
    if ( hwndParent == NULLHANDLE ) return;

    // Get the parent window's size
    WinQueryWindowRect( hwndParent, &rcl );
    lHeight = rcl.yTop;

    // If there's no text, just set the width to 0 and return
    if ( !pCtl->puszText || !pCtl->usTextLen ) {
        WinSetWindowPos( hwndParent, HWND_TOP, 0, 0, 0, lHeight, SWP_SIZE );
        return;
    }

    // Get the current control area
    WinQueryWindowRect( hwnd, &rcl );

    // Set up the font face
    if (( GpiCreateLogFont( hps, NULL, 1L, &(pCtl->fattrs) )) == GPI_ERROR ) return;

    // Set the text size
    lCell = rcl.yTop - rcl.yBottom - 2;
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );
    // Adjust the cell height to take the maximum character bbox into account
    dSizeAdjust = (double)(fm.lEmHeight) / fm.lMaxBaselineExt;
    if ( dSizeAdjust > 0 ) {
        if ( dSizeAdjust > 1 ) dSizeAdjust = 1;
        else if ( dSizeAdjust < 0.75 ) dSizeAdjust = 0.75;
        lCell *= dSizeAdjust;
    }
    sfCell.cy = MAKEFIXED( lCell, 0 );
    sfCell.cx = sfCell.cy;
    GpiSetCharBox( hps, &sfCell );
    GpiSetCharSet( hps, 1L );
    GpiQueryFontMetrics( hps, sizeof(FONTMETRICS), &fm );
    pCtl->lCursorHeight = rcl.yTop;

    // Since we're only interested in the string width, the y position is irrelevant
    ptl.x = 1;
    ptl.y = 1;
    GpiSetTextAlignment( hps, TA_LEFT, TA_BOTTOM );
    pchText = (PCH) pCtl->puszText;
    cb = UniStrlen( pCtl->puszText ) * 2;
    GpiQueryTextBox( hps, cb, pchText, TXTBOX_COUNT, aptl );

    // Update the window width
    WinSetWindowPos( hwndParent, HWND_TOP, 0, 0, aptl[ TXTBOX_CONCAT ].x + fm.lEmInc, lHeight, SWP_SIZE );
}

