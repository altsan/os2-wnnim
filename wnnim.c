/****************************************************************************
 * wnnim.c                                                                  *
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
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uconv.h>

#define INCL_LOADEXCEPTQ
#include "exceptq.h"

#include "wnnhook.h"
#include "codepage.h"
#include "ids.h"
#include "wnnim.h"
#include "wnnclient.h"
#include "convwin.h"
#include "settings.h"
#include "clipfuncs.h"


//#define PRESERVE_CLIPBOARD 1


// --------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//

MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             AcceptClause( HWND hwnd );
MRESULT EXPENTRY ButtonProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             ClearInputBuffer( void );
MRESULT EXPENTRY ClientWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
INT              ConvertClauseText( USHORT usPhrase );
void             DoClauseConversion( HWND hwnd );
void             DoPhraseConversion( HWND hwnd );
void             Draw3DBorder( HPS hps, RECTL rcl, BOOL fInset );
void             EnterPhrase( HWND hwnd, USHORT usWhich );
VOID APIENTRY    ExeTrap( void );
void             DismissConversionWindow( HWND hwnd );
void             NextInputMode( HWND hwnd );
void             PaintIMEButton( PUSERBUTTON pBtnData );
MRESULT          PassStdEvent( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             ProcessCharacter( HWND hwnd, HWND hwndSource, MPARAM mp1, MPARAM mp2 );
void             SendCharacter( HWND hwndSource, PSZ pszBuffer, UniChar *puszBuffer );
BOOL             SetConversionWindow( HWND hwnd, HWND hwndSource );
void             SetInputMode( HWND hwnd, USHORT usNewMode );
void             SetTopmost( HWND hwnd );
BOOL             SetupDBCSLanguage( USHORT usLangMode );
void             SetupWindow( HWND hwnd );
void             SizeWindow( HWND hwnd, POINTL ptl );
void             SupplyCharacter( HWND hwnd, HWND hwndSource, BYTE bStatus );
MRESULT EXPENTRY StaticTextProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void             ToggleKanjiConversion( HWND hwnd );
void             ToggleInputConversion( HWND hwnd );
void             UpdateStatus( HWND hwnd );


IMCLIENTDATA global = {0};      // our window's global data

HATOMTBL    g_hSATbl;           // handle to system atom table
ATOM        g_cfUnicode;        // "text/unicode" clipboard format atom

// Subclassed window procedures
PFNWP pfnBtnProc;
PFNWP pfnTxtProc;




// ==========================================================================
// IMPLEMENTATION
// ==========================================================================

/* ------------------------------------------------------------------------- *
 * ClearInputBuffer                                                          *
 *                                                                           *
 * Clear/reset the input conversion buffers.  This should always be done     *
 * after calling SendCharacter.  Effectively this resets the current input   *
 * conversion to the empty state.  Note that this does not affect the clause *
 * conversion buffer.                                                        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void ClearInputBuffer( void )
{
    memset( global.szRomaji, 0, sizeof( global.szRomaji ));
    memset( global.uszKana, 0, sizeof( global.uszKana ));
}


/* ------------------------------------------------------------------------- *
 * SetConversionWindow                                                       *
 *                                                                           *
 * Open the clause conversion overlay window and position it over the source *
 * window.                                                                   *
 * ------------------------------------------------------------------------- */
BOOL SetConversionWindow( HWND hwnd, HWND hwndSource )
{
    CWCTLDATA   ctldata = {0};
    FONTMETRICS fm = {0};
    CHAR        szFontPP[ FACESIZE + 4 ];
    HPS         hps;
    BOOL        fGotPos,
                fGotHeight;
    PID         pid;
    CURSORINFO  ci;
    POINTL      ptl;
    LONG        lClr,
                lTxtHeight;
    USHORT      usRC;


    if ( !hwndSource ) return FALSE;
    if (( pShared->fsMode & MODE_CJK_ENTRY ) && global.hwndClause )
        return TRUE;

    // Default font and line height (for fallback)
    // (the point size is irrelevant but the PP format expects it)
    if ( global.szInputFont[0] )
        sprintf( szFontPP, "10.%.31s", global.szInputFont );
    else
        strcpy( szFontPP, "10.Times New Roman MT 30");
    lTxtHeight = 40;

    // Create the conversion window if it doesn't already exist
    if ( global.hwndClause == NULLHANDLE ) {
        ctldata.cb = sizeof( CWCTLDATA );
        ctldata.fsMode = pShared->fsMode & 0xF00;
        ctldata.flFlags = CWS_BORDER;
        global.hwndClause = WinCreateWindow( HWND_DESKTOP, WC_WNNIMCONVWIN, "",
                                             0L, 0, 0, 0, 0, hwnd, HWND_TOP,
                                             IDD_CLAUSE, &ctldata, NULL );
        lClr = SYSCLR_WINDOW;
        WinSetPresParam( global.hwndClause, PP_BACKGROUNDCOLORINDEX,
                         sizeof( lClr ), &lClr );
        lClr = SYSCLR_WINDOWTEXT;
        WinSetPresParam( global.hwndClause, PP_FOREGROUNDCOLORINDEX,
                         sizeof( lClr ), &lClr );
    }
    if ( !global.hwndClause ) return FALSE;    // TODO error message?

    // Tell the conversion window who's using it
    WinSendMsg( global.hwndClause, CWM_SETINPUTWINDOW,
                MPFROMP( hwndSource ), 0L );

    ptl.x = 1;
    ptl.y = 1;
    fGotPos = FALSE;
    fGotHeight = FALSE;

    // Now determine where to position it. First we try asking the app...
    if ( global.pRclConv ) {
        if (( NO_ERROR == WinQueryWindowProcess( hwndSource, &pid, NULL )) &&
            ( NO_ERROR == DosGiveSharedMem( global.pRclConv, pid, fPERM )))
        {
            usRC = (USHORT) WinSendMsg( hwndSource, WM_QUERYCONVERTPOS,
                                        MPFROMP( global.pRclConv ), 0L );
            if ( usRC == QCP_NOCONVERT ) return FALSE;
        }
        if ( !( global.pRclConv->xLeft == -1 ) &&
             !( global.pRclConv->yBottom == -1 ))
        {
            ptl.x = global.pRclConv->xLeft;
            ptl.y = global.pRclConv->yBottom;
            fGotPos = TRUE;
            if (( global.pRclConv->yTop - global.pRclConv->yBottom ) > 4 ) {
                lTxtHeight = ( global.pRclConv->yTop - global.pRclConv->yBottom ) + 2;
                fGotHeight = TRUE;
            }
        }
    }

    // If the app wouldn't tell us the convert position, try & query the cursor
    if ( !fGotPos && WinQueryCursorInfo( HWND_DESKTOP, &ci )) {
        if ( ci.hwnd == hwndSource )
        {
            ptl.x = ci.x;
            ptl.y = ci.y;
            fGotPos = TRUE;
            if ( ci.cy > 4 ) {
                lTxtHeight = ci.cy + 2;
                fGotHeight = TRUE;
            }
        }
    }

    if ( fGotPos ) {
        WinMapWindowPoints( hwndSource, HWND_DESKTOP, &ptl, 1 );

        // Try and set the font and window (line) height to match the source window
        // (this doesn't really work very well but in some cases it's semi-useful)
        if ( !fGotHeight ) {
            hps = WinGetPS( hwndSource );
            if ( GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm )) {
                lTxtHeight = fm.lMaxBaselineExt + 2;
                // Note: the point size here is ignored by the conversion window
#if 0           // ALT - better to use just the user-configured font
                sprintf( szFontPP, "%d.%s", fm.sNominalPointSize, fm.szFacename );
#endif
            }
            WinReleasePS( hps );
        }
    }

    // Set the conversion window font. This sets the face, but the window
    // will set the point size itself based on the window height.
    WinSetPresParam( global.hwndClause, PP_FONTNAMESIZE,
                     strlen( szFontPP ), szFontPP );

    // Now get the conversion window's actual font metrics and use to set the
    // position relative to the application cursor.  (We want it to line up so
    // that the text baselines match as closely as possible.)
    if ( (BOOL)WinSendMsg( global.hwndClause, CWM_QUERYFONTMETRICS,
                           MPFROMP( &fm ), MPFROMLONG( lTxtHeight )))
    {
        // The application's cursor position is usually at or around the
        // underscore position, but it kind of depends.  Also, the actual
        // font metrics can vary quite widely, so this is a bit of a fudge...
        ptl.y -= min( fm.lLowerCaseDescent - fm.lUnderscorePosition, fm.lEmHeight / 5 );
        if ( ptl.y < 0 ) ptl.y = 0;
    }

    // Place it over the source window at the position indicated
    WinSetWindowPos( global.hwndClause, HWND_TOP,
                     ptl.x, ptl.y, 0, lTxtHeight,
                     SWP_MOVE | SWP_SHOW | SWP_SIZE );

    pShared->fsMode |= MODE_CJK_ENTRY;
    WinSetFocus( HWND_DESKTOP, global.hwndClause );
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * DismissConversionWindow                                                   *
 *                                                                           *
 * Hide the conversion window and clear its contents.                        *
 * ------------------------------------------------------------------------- */
void DismissConversionWindow( HWND hwnd )
{
    // Clear the conversion window text
    WinSendMsg( global.hwndClause, CWM_SETTEXT,
                MPFROM2SHORT( CWT_ALL, 0 ), MPFROMP( NULL ));

    // De-associate the conversion window from the current application
    WinSendMsg( global.hwndClause, CWM_SETINPUTWINDOW, 0L, 0L );

    // Hide the window
    WinShowWindow( global.hwndClause, FALSE );
    if ( global.hwndInput )
        WinSetFocus( HWND_DESKTOP, global.hwndInput );

    // Reset the IME conversion state
    ClearConversion( global.pSession );
    global.fsClause = 0;

    // Update the input mode status
    pShared->fsMode &= ~MODE_CJK_ENTRY;
    pShared->fsMode &= ~MODE_CJK_PHRASE;
}


/* ------------------------------------------------------------------------- *
 * PasteCharacters                                                           *
 *                                                                           *
 * This function attempts to send the buffer contents to the source window   *
 * by pasting it to the clipboard in text/unicode format, then sending the   *
 * source window a Shift+Insert key event.  This should only be used with    *
 * specific applications where we know this is likely to work.               *
 *                                                                           *
 * If there is data already on the clipboard in one of the standard OS/2     *
 * formats, it will be preserved.  Other formats that we don't know about,   *
 * however, will be lost.                                                    *
 * ------------------------------------------------------------------------- */
BOOL PasteCharacters( HWND hwndSource, UniChar *puszBuffer )
{
#ifdef PRESERVE_CLIPBOARD
    // Saved clipboard information
    ULONG    ulFmtBMP    = 0,
             ulFmtDspBMP = 0,
             ulFmtMF     = 0,
             ulFmtDspMF  = 0,
             ulFmtTXT    = 0,
             ulFmtDspTXT = 0,
             ulFmtUTXT   = 0;
    HMF      hMF         = NULLHANDLE,
             hDspMF      = NULLHANDLE,
             hTmpMF      = NULLHANDLE;
    HBITMAP  hBMP        = NULLHANDLE,
             hDspBMP     = NULLHANDLE,
             hTmpBMP     = NULLHANDLE;
    PSZ      pszTXT      = NULL,
             pszDspTXT   = NULL,
             pszTmpTXT   = NULL;
    UniChar *puszTXT     = NULL,
            *puszTmpTXT  = NULL;
#endif

    if ( !g_cfUnicode ) return FALSE;

    if ( ! WinOpenClipbrd( global.hab )) return FALSE;

#ifdef PRESERVE_CLIPBOARD
    // Save existing clipboard contents (this only works for standard formats)
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_BITMAP, &ulFmtBMP )) {
        hTmpBMP = (HBITMAP) WinQueryClipbrdData( global.hab, CF_BITMAP );
        if ( hTmpBMP )
            hBMP = CopyBitmap( global.hab, hTmpBMP );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_DSPBITMAP, &ulFmtDspBMP )) {
        hTmpBMP = (HBITMAP) WinQueryClipbrdData( global.hab, CF_DSPBITMAP );
        if ( hTmpBMP )
            hDspBMP = CopyBitmap( global.hab, hTmpBMP );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_METAFILE, &ulFmtMF )) {
        hTmpMF = (HMF) WinQueryClipbrdData( global.hab, CF_METAFILE );
        if ( hTmpMF )
            hMF = GpiCopyMetaFile( hTmpMF );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_DSPMETAFILE, &ulFmtDspMF )) {
        hTmpMF = (HMF) WinQueryClipbrdData( global.hab, CF_DSPMETAFILE );
        if ( hTmpMF )
            hDspMF = GpiCopyMetaFile( hTmpMF );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_TEXT, &ulFmtTXT )) {
        pszTmpTXT = (PSZ) WinQueryClipbrdData( global.hab, CF_TEXT );
        if ( pszTmpTXT )
            pszTXT = strdup( pszTmpTXT );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, CF_DSPTEXT, &ulFmtDspTXT )) {
        pszTmpTXT = (PSZ) WinQueryClipbrdData( global.hab, CF_DSPTEXT );
        if ( pszTmpTXT )
            pszDspTXT = strdup( pszTmpTXT );
    }
    if ( WinQueryClipbrdFmtInfo( global.hab, g_cfUnicode, &ulFmtUTXT )) {
        puszTmpTXT = (UniChar *) WinQueryClipbrdData( global.hab, g_cfUnicode );
        if ( pszTmpTXT ) {
            puszTXT = (UniChar *) calloc( UniStrlen( puszTmpTXT ), sizeof( UniChar ));
            UniStrcpy( puszTXT, puszTmpTXT );
        }
    }
#endif

    WinEmptyClipbrd( global.hab );

    // Place the UCS-2 string on the clipboard as "text/unicode"
    if ( ClipPutUniText( global.hab, puszBuffer, g_cfUnicode )) {
        // Now close the clipboard and send Shift+Ins to the app
        WinCloseClipbrd( global.hab );
        WinSendMsg( hwndSource, WM_CHAR,
                    MPFROMSH2CH( KC_VIRTUALKEY | KC_SHIFT, 1, 0 ),
                    MPFROM2SHORT( 0, VK_INSERT ));
        // Reopen the clipboard
#ifdef PRESERVE_CLIPBOARD
        if ( ! WinOpenClipbrd( global.hab )) return TRUE;
#endif
    }
#ifdef PRESERVE_CLIPBOARD
    // Unfortunately this doesn't work reliably.  The problem is that the app may
    // not actually process the paste event until after this takes place.  This
    // seems to be timing sensitive, and since we don't have access to the app's
    // side of the conversation, semaphores and IPC tricks aren't an option.
    // All we can really do is leave our IME text in the clipboard so that the app
    // pastes it once it's ready (which means we can't save the old clipboard data).

    WinEmptyClipbrd( global.hab );
    // Finish up by restoring any saved clipboard contents
    if ( hBMP != NULLHANDLE )
        WinSetClipbrdData( global.hab, (ULONG) hBMP, CF_BITMAP, ulFmtBMP );
    if ( hDspBMP != NULLHANDLE )
        WinSetClipbrdData( global.hab, (ULONG) hDspBMP, CF_DSPBITMAP, ulFmtDspBMP );
    if ( hMF != NULLHANDLE )
        WinSetClipbrdData( global.hab, (ULONG) hMF, CF_METAFILE, ulFmtMF );
    if ( hDspMF != NULLHANDLE )
        WinSetClipbrdData( global.hab, (ULONG) hDspMF, CF_DSPMETAFILE, ulFmtDspMF );
    if ( pszTXT != NULL )
        ClipPutText( global.hab, pszTXT, CF_TEXT );
    if ( pszDspTXT != NULL )
        ClipPutText( global.hab, pszDspTXT, CF_DSPTEXT );
    if ( puszTXT != NULL )
        ClipPutUniText( global.hab, puszTXT, g_cfUnicode );
    WinCloseClipbrd( global.hab );
#endif

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * SendCharacter                                                             *
 *                                                                           *
 * Send the contents of the specified character buffer to the source window. *
 * Typically this will cause the characters to be inserted at the current    *
 * position, but in practice it's up to the application to decide what to do *
 * with it.                                                                  *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND     hwndSource: HWND of source (application) window.               *
 *   PSZ      pszBuffer : Character buffer whose contents will be sent.      *
 *   UniChar *puszBuffer: Character buffer in unconverted UCS-2 format       *
 *                        (needed for our clipboard workaround, see below).  *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SendCharacter( HWND hwndSource, PSZ pszBuffer, UniChar *puszBuffer )
{
    USHORT i,
           usLen,
           usChar;
    CHAR   achClassName[ 50 ] = {0};
    BOOL   fWorkAround = FALSE,
           fUseClipboard = FALSE;


    // Some custom input windows can't handle combined double bytes, so
    // we have a workaround to send each byte separately.  OTOH, the
    // standard PM controls (and many others) need the double-byte logic
    // for input to work properly when running under a DBCS codepage.
    // Also, the workaround only works for applications which interpret
    // characters as byte values under the current codepage.
    //
    // Our solution is to use the double-byte logic by default but enable
    // the workaround for apps (or rather, window classes) which are known
    // to be problematic.

    if ( WinQueryClassName( hwndSource, sizeof( achClassName ), achClassName ) > 0 )
    {

        // MED (MrED) text editor
        if ( strcmp( achClassName, "MRED_BUFWIN_CLASS") == 0 )
            fWorkAround = TRUE;

        // OpenOffice/StarOffice (the normal workaround won't work, since
        // OpenOffice converts internally to Unicode before displaying).
        // Instead, we cheat by putting the UCS-2 text onto the clipboard and
        // sending a Paste command (Shift+Ins).
        else if ( strcmp( achClassName, "SALFRAME") == 0 )
            fUseClipboard = TRUE;

    }

    if ( fUseClipboard && puszBuffer ) {
        PasteCharacters( hwndSource, puszBuffer );
        return;
    }

    usLen = strlen( pszBuffer );
    for ( i = 0; i < usLen; i++ ) {
        usChar = (USHORT) pszBuffer[ i ];

        if ( !fWorkAround && IsDBCSLeadByte( usChar, global.dbcs ))
            usChar |= pszBuffer[ ++i ] << 0x8;

        WinSendMsg( hwndSource, WM_CHAR,
                    MPFROMSH2CH( KC_CHAR, 1, 0 ),
                    MPFROM2SHORT( usChar, 0 ));

        if ( i > 2000 ) break;     // sanity check
    }

    // Force a window redraw if workaround was used
    if ( fWorkAround ) WinInvalidateRect( hwndSource, NULL, FALSE );

}


/* ------------------------------------------------------------------------- *
 * SupplyCharacter                                                           *
 *                                                                           *
 * Output converted input buffer.  If CJK conversion is active, add it to    *
 * the clause buffer and display it in the overlay window.  (TODO: or if     *
 * it's a 'candidate' character, i.e. valid but potentially modifiable, put  *
 * it in the pending buffer and display it in the overlay window.)           *
 * Otherwise, send it directly to the target window, and clear all buffers.  *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void SupplyCharacter( HWND hwnd, HWND hwndSource, BYTE bStatus )
{
    static CHAR szKana[ MAX_KANA_BUFZ ];

    memset( szKana, 0, sizeof( szKana ));

    /* TODO for Korean only
    if ( bStatus == KANA_CANDIDATE )
        UniStrncpy( global.suPending, global.uszKana, MAX_KANA_BUFZ );
        // TODO update/display overlay
    else
    */
    if (( pShared->fsMode & MODE_CJK ) &&
        ( SetConversionWindow( hwnd, hwndSource )))
    {
        // Clause text is changing, so reset the conversion state
        if (( global.fsClause & CLAUSE_READY ) == CLAUSE_READY ) {
            ClearConversion( global.pSession );
            pShared->fsMode &= ~MODE_CJK_PHRASE;
            global.fsClause &= ~CLAUSE_READY;
        }

        // Add character to clause conversion window
        WinSendMsg( global.hwndClause, CWM_ADDCHAR,
                    MPFROMSHORT( UniStrlen( global.uszKana )),
                    MPFROMP( global.uszKana ));
        ClearInputBuffer();
    }
    else
    {
        // Convert kana buffer to output codepage and send to application
        StrConvert( (PCH)(global.uszKana), szKana, NULL, global.uconvOut );
        SendCharacter( hwndSource, szKana, global.uszKana );
        ClearInputBuffer();
    }
}


/* ------------------------------------------------------------------------- *
 * ProcessCharacter                                                          *
 *                                                                           *
 * Process an input character (byte) from the input hook and decide how to   *
 * deal with it.  If we made it this far, then we know that the IME is       *
 * enabled.  Here, we add the character to the romaji buffer, then call the  *
 * input conversion routine to see if it can be converted.                   *
 *                                                                           *
 * The romaji buffer accumulates characters until either a valid conversion  *
 * is possible, the maximum length is reached, or an illegal sequence is     *
 * reported.  It can be in one of five states:                               *
 *                                                                           *
 *  A. empty (not applicable if we've reached this point)                    *
 *  B. pending (contains a partial sequence which is potentially valid)      *
 *  C. complete (contains a valid romaji sequence ready for conversion)      *
 *  D. candidate (contains a valid romaji                                    *
 *  E. invalid (contains a sequence that is not nor could ever become valid) *
 *                                                                           *
 * A does not apply at this point.  In the case of B, we simply keep the     *
 * buffer and continue.  For the other three possibilities we call the       *
 * SupplyCharacter() function and let it decide how to proceed.              *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND   hwnd      : Our own window handle.                               *
 *   HWND   hwndSource: Handle of source (application) window.               *
 *   MPARAM mp1       : mp1 of original WM_CHAR message.                     *
 *   MPARAM mp2       : mp2 of original WM_CHAR message.                     *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void ProcessCharacter( HWND hwnd, HWND hwndSource, MPARAM mp1, MPARAM mp2 )
{
    UCHAR szChar[ 2 ];
    BYTE  bStatus;

    if (( hwndSource != global.hwndInput ) && ( hwndSource != global.hwndClause )) {
        // Source window changed, clear any existing buffers.
        global.hwndInput = hwndSource;
        ClearInputBuffer();
        DismissConversionWindow( hwnd );
        // ClearClauseBuffer();
    }

    szChar[ 0 ] = (UCHAR) SHORT1FROMMP( mp2 );
    szChar[ 1 ] = 0;
    strncat( global.szRomaji, szChar, sizeof(global.szRomaji) - 1 );

    if ( IS_INPUT_MODE( pShared->fsMode, MODE_FULLWIDTH ))
        bStatus = ConvertFullWidth( global.szRomaji, global.uszKana,
                                    sizeof( global.uszKana ) / sizeof( UniChar )) > 0 ?
                  KANA_COMPLETE: KANA_INVALID;
    else
        bStatus = ConvertPhonetic( pShared->fsMode );
    if ( bStatus != KANA_PENDING ) {
        SupplyCharacter( hwnd, hwndSource, bStatus );
    }
    // otherwise just keep the buffers and continue
}


/* ------------------------------------------------------------------------- *
 * AcceptClause                                                              *
 *                                                                           *
 * Accept the current clause text as shown in the overlay window and send it *
 * to the source application window.  Then dismiss the overlay window.       *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void AcceptClause( HWND hwnd )
{
    UniChar *puszClause;
    PSZ      pszClause;
    USHORT   usLen;

    if ( !global.hwndClause || !global.hwndInput ) return;

    usLen = (USHORT) WinSendMsg( global.hwndClause, CWM_QUERYTEXTLENGTH,
                                 MPFROMSHORT( CWT_ALL ), 0 );
    if ( usLen ) {
        puszClause = (UniChar *) calloc( usLen + 1, sizeof( UniChar ));
        if ( puszClause ) {
            if ( WinSendMsg( global.hwndClause, CWM_GETTEXT,
                             MPFROM2SHORT( CWT_ALL, usLen+1 ),
                             MPFROMP( puszClause )))
            {
                DismissConversionWindow( hwnd );
                usLen *= 4;
                pszClause = (PSZ) calloc( usLen + 1, sizeof( char ));
                if ( pszClause ) {
                    StrConvert( (PCH) puszClause, pszClause, NULL, global.uconvOut );
                    SendCharacter( global.hwndInput, pszClause, puszClause );
                    free( pszClause );
                }
            }
            free( puszClause );
        }
    }
    else
        DismissConversionWindow( hwnd );
}


/* ------------------------------------------------------------------------- *
 * ConvertClauseText                                                         *
 *                                                                           *
 * ------------------------------------------------------------------------- */
INT ConvertClauseText( USHORT usPhrase )
{
    UniChar *puszText;      // Current clause/phrase text from conversion window
    USHORT   usLen;
    INT      rc;

    usLen = (USHORT) WinSendMsg( global.hwndClause, CWM_QUERYTEXTLENGTH,
                                 MPFROMSHORT( usPhrase ), 0 );
    if ( ! usLen ) return CONV_FAILED;

    puszText = (UniChar *) calloc( usLen + 1, sizeof( UniChar ));
    if ( !puszText ) return CONV_FAILED;

    if ( ! WinSendMsg( global.hwndClause, CWM_GETTEXT,
                       MPFROM2SHORT( usPhrase, usLen+1 ),
                       MPFROMP( puszText )))
        rc = CONV_FAILED;
    else {
        if ( usPhrase == CWT_ALL )
            rc = ConvertClause( global.pSession, puszText );
        else
            rc = ConvertPhrase( global.pSession, puszText );
    }

    free( puszText );
    return rc;
}


/* ------------------------------------------------------------------------- *
 * DoClauseConversion                                                        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void DoClauseConversion( HWND hwnd )
{
    UniChar *puszCandidate;     // Current conversion candidate
    INT      rc;

    if ( !global.hwndClause || !global.hwndInput ) return;

    pShared->fsMode &= ~MODE_CJK_PHRASE;
    global.fsClause &= ~CLAUSE_PHRASE_READY;

#if 0
    // See if we already initialized this clause (by getting the phrase count)
    rc = GetPhraseCount( global.pSession );
    if ( rc == CONV_CONNECT ) {
        // Lost connection, try (once) to reestablish
        rc = InitConversionMethod( NULL, pShared->fsMode & 0xF00,
                                   (PVOID *) &(global.pSession) );
        if ( rc != CONV_CONNECT )
            rc = GetPhraseCount( global.pSession );
        if ( rc == CONV_CONNECT ) {
            // No dice, give up
            ErrorPopup( hwnd, global.szEngineError );
            return;
        }
    }
    if ( rc < 1 ) {
#else
    if ( (global.fsClause & CLAUSE_READY) != CLAUSE_READY ) {
#endif
        // This is a new clause, not yet initialized.  So do that now.

        rc = ConvertClauseText( CWT_ALL );
        if ( rc != CONV_OK ) {
            ErrorPopup( hwnd, global.szEngineError );
            return;
        }
        global.fsClause |= CLAUSE_READY;

        // Now set up the candidate list
        rc = PrepareCandidates( global.pSession );
        if ( rc == CONV_CONNECT ) {
            ErrorPopup( hwnd, global.szEngineError );
            return;
        }

        // The clause has now had its initial processing by the IME engine.
        // That means we have (a) candidate conversions for the entire clause,
        // and (b) a breakdown of component phrases within the clause.
        // The default conversion is now available to be retrieved with
        // GetConvertedString().
    }

    else            // Already converted, just advance to the next candidate
        SetCandidate( global.pSession, TRUE );

    // Get the next candidate conversion
    if ( CONV_OK == GetConvertedString( global.pSession, 0, -1,
                                        FALSE, &puszCandidate ))
    {
        WinSendMsg( global.hwndClause, CWM_SETTEXT,
                    MPFROM2SHORT( CWT_ALL, UniStrlen( puszCandidate )),
                    MPFROMP( puszCandidate ));
        free ( puszCandidate );
    }
}


/* ------------------------------------------------------------------------- *
 * DoPhraseConversion                                                        *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void DoPhraseConversion( HWND hwnd )
{
    UniChar *puszCandidate;     // Current conversion candidate
    USHORT   usPhrase;
    INT      rc;

    if ( !global.hwndClause || !global.hwndInput ) return;

    // Get the selected phrase number
    usPhrase = (USHORT) WinSendMsg( global.hwndClause,
                                    CWM_GETSELECTEDPHRASE, 0L, 0L );
    if ( usPhrase == CWT_NONE ) return;

    if ((( global.fsClause & 0xFF00 ) != usPhrase ) ||
        (( global.fsClause & CLAUSE_PHRASE_READY ) != CLAUSE_PHRASE_READY ))
    {
        // This phrase has not been converted yet, so do that now

        if ( CONV_OK != ConvertClauseText( usPhrase )) {
            ErrorPopup( hwnd, global.szEngineError );
            return;
        }
        global.fsClause |= CLAUSE_PHRASE_READY;
        global.fsClause |= usPhrase << 16;

        rc = PrepareCandidates( global.pSession );
        if ( rc == CONV_CONNECT ) {
            ErrorPopup( hwnd, global.szEngineError );
            return;
        }
    }
    else        // Already converted, just advance to the next candidate
        SetCandidate( global.pSession, TRUE );

    // Get the converted string
    if ( CONV_OK == GetConvertedString( global.pSession, 0, -1,
                                        FALSE, &puszCandidate ))
    {
        WinSendMsg( global.hwndClause, CWM_SETTEXT,
                    MPFROM2SHORT( usPhrase, UniStrlen( puszCandidate )),
                    MPFROMP( puszCandidate ));
        free ( puszCandidate );
    }
}


/* ------------------------------------------------------------------------- *
 * EnterPhrase                                                               *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void EnterPhrase( HWND hwnd, USHORT usWhich )
{
    UniChar *puszPhrase;
    USHORT  usPhrase;
    USHORT  ausPhrases[ MAX_PHRASES ];
    BOOL    fRC;
    INT     iCount,
            iLen,
            i;

    if ( !global.hwndClause || !global.hwndInput ) return;
    if (( global.fsClause & CLAUSE_READY ) != CLAUSE_READY ) return;

    if (( pShared->fsMode & MODE_CJK_PHRASE ) != MODE_CJK_PHRASE ) {
        // Entering phrase mode, so set the phrase boundaries

        iCount = GetPhraseCount( global.pSession );
        if ( iCount == 0 ) return;
        if ( iCount > MAX_PHRASES ) iCount = MAX_PHRASES;

        memset( ausPhrases, 0, sizeof( ausPhrases ));

        _PmpfF(("Phrase mode activated."));

        //_PmpfF(("Setting boundaries for %d phrases:", iCount ));

        for ( i = 0; i < iCount; i++ ) {
            if ( CONV_OK == GetConvertedString( global.pSession, i, i+1,
                                                FALSE, &puszPhrase ))
            {
                iLen = UniStrlen( puszPhrase );
                ausPhrases[ i ] = i? ( ausPhrases[ i-1 ] + iLen ):
                                     ( iLen - 1 );

//                _PmpfF((" %d) %d", i, ausPhrases[ i ] ));

                free( puszPhrase );
            }
            else {
                _PmpfF((" %d) Failed to get converted string!", i ));
                return;
            }
        }
        fRC = (BOOL) WinSendMsg( global.hwndClause, CWM_SETPHRASES,
                                 MPFROMSHORT( iCount ), MPFROMP( ausPhrases ));
        if ( !fRC ) {
            _PmpfF(("Failed to set phrase boundaries!"));
            return;
        }

        pShared->fsMode |= MODE_CJK_PHRASE;
    }

    usPhrase = (USHORT) WinSendMsg( global.hwndClause, CWM_SELECTPHRASE,
                                    MPFROMSHORT( usWhich ), 0L );

    _PmpfF(("Phrase %d selected.", usPhrase ));

}


/* ------------------------------------------------------------------------- *
 * CentreWindow                                                              *
 *                                                                           *
 * Centres one window relative to another (or to the screen).  The window    *
 * will always be placed on top of the z-order (HWND_TOP).                   *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND  hwndCentre  : the window to be centred                            *
 *   HWND  hwndRelative: the window relative to which hwndCentre will be     *
 *                       centred, or NULLHANDLE to centre on the screen      *
 *   ULONG flFlags     : additional flags for WinSetWindowPos (SWP_MOVE is   *
 *                       always assumed                                      *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void CentreWindow( HWND hwndCentre, HWND hwndRelative, ULONG flFlags )
{
    LONG x, y,       // x & y coordinates of hwndCentre (relative to hwndRelative)
         ox, oy,     // x & y offsets (i.e. coordinates of hwndRelative)
         rcx, rcy;   // width & height of hwndRelative
    SWP  wp;         // window-position structure


    if ( !hwndRelative ) {
        rcx = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
        rcy = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
        ox = 0;
        oy = 0;
    }
    else {
        if ( ! WinQueryWindowPos( hwndRelative, &wp )) return;
        rcx = wp.cx;
        rcy = wp.cy;
        ox = wp.x;
        oy = wp.y;
    }
    if ( WinQueryWindowPos( hwndCentre, &wp )) {
        x = ( rcx / 2 ) - ( wp.cx / 2 );
        y = ( rcy / 2 ) - ( wp.cy / 2 );
        WinSetWindowPos( hwndCentre, HWND_TOP,
                         x + ox, y + oy, wp.cx, wp.cy, SWP_MOVE | flFlags );
    }
}


/* ------------------------------------------------------------------------- *
 * Window procedure for 'About' (product info) dialog.                       *
 * See OS/2 PM reference for a description of input and output.              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    CHAR achVersion[ MAX_VERSTRZ ];
    CHAR achBuf[ 256 ];

    switch ( msg ) {
        case WM_INITDLG:
            sprintf( achVersion, "Version %s", SZ_VERSION );
            WinSetDlgItemText( hwnd, IDD_VERSION, achVersion );
            sprintf( achBuf, "Copyright (C) %s Alexander Taylor.", SZ_COPYRIGHT );
            strncat( achBuf, "\r\n\nWnnIM for OS/2 is free software published under the terms of the GNU General Public License, version 2.  ", 255 );
            strncat( achBuf, "See the accompanying documentation for details.", 255 );
            WinSetDlgItemText( hwnd, IDD_NOTICES, achBuf );
            CentreWindow( hwnd, NULLHANDLE, SWP_SHOW );
            break;
    }
    return WinDefDlgProc (hwnd, msg, mp1, mp2) ;
}


/* ------------------------------------------------------------------------- *
 * Handle certain events in a subclassed control that we want to pass up to  *
 * that control's owner.  The main purpose of this is to make sure drag and  *
 * context-menu events on the control are handled by the dialog, rather than *
 * getting eaten by the control.                                             *
 *                                                                           *
 * Input/output are as per window procedure; see OS/2 PM reference.          *
 * ------------------------------------------------------------------------- */
MRESULT PassStdEvent( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    POINTL ptl;
    POINTS pts;
    HWND   hwndApp;

    ptl.x = SHORT1FROMMP( mp1 );
    ptl.y = SHORT2FROMMP( mp1 );
    hwndApp = WinQueryWindow( hwnd, QW_OWNER );
    WinMapWindowPoints( hwnd, hwndApp, &ptl, 1 );
    pts.x = (SHORT) ptl.x;
    pts.y = (SHORT) ptl.y;
    return (MRESULT) WinPostMsg( hwndApp, msg, MPFROM2SHORT(pts.x, pts.y), mp2 );
}


/* ------------------------------------------------------------------------- *
 * ButtonProc()                                                              *
 *                                                                           *
 * Subclassed window procedure for the button controls.                      *
 * See OS/2 PM reference for a description of input and output.              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ButtonProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg ) {
        // pass these messages up to the owner
        case WM_BEGINDRAG:
        case WM_CONTEXTMENU:
            return PassStdEvent( hwnd, msg, mp1, mp2 );
    }
    return (MRESULT) pfnBtnProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * StaticTextProc()                                                          *
 *                                                                           *
 * Subclassed window procedure for the static text control.                  *
 * See OS/2 PM reference for a description of input and output.              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY StaticTextProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    ULONG id, cb, lClr;
    CHAR  szFont[ FACESIZE+4 ];

    switch( msg ) {
        // pass these messages up to the owner after converting the pointer pos
        case WM_BEGINDRAG:
        case WM_CONTEXTMENU:
            return PassStdEvent( hwnd, msg, mp1, mp2 );

        // pass colour and font changes up to the owner
        case WM_PRESPARAMCHANGED:
            id = (ULONG) mp1;
            switch ( id ) {
                case PP_BACKGROUNDCOLORINDEX:
                case PP_FOREGROUNDCOLORINDEX:
                case PP_BACKGROUNDCOLOR:
                case PP_FOREGROUNDCOLOR:
                    cb = WinQueryPresParam( hwnd, id, 0,
                                            NULL, sizeof( lClr ), &lClr, 0 );
                    if ( cb ) WinSetPresParam( WinQueryWindow( hwnd, QW_OWNER ),
                                               id, sizeof( lClr ), &lClr );
                    return 0;

                case PP_FONTNAMESIZE:
                    cb = WinQueryPresParam( hwnd, PP_FONTNAMESIZE, 0, NULL,
                                            sizeof( szFont ), szFont, 0 );
                    if ( cb ) WinSetPresParam( WinQueryWindow( hwnd, QW_OWNER ),
                                               id, strlen( szFont ) + 1, szFont );
                    return 0;
            }
            break;

    }
    return (MRESULT) pfnTxtProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * SetTopmost                                                                *
 *                                                                           *
 * Toggle the 'always on top' (float) setting of the window.                 *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SetTopmost( HWND hwnd )
{
    USHORT usState;
    ULONG  fl;
    BOOL   fCheck;

    if ( !global.hwndMenu ) return;
    usState = (USHORT) WinSendMsg( global.hwndMenu, MM_QUERYITEMATTR,
                                   MPFROM2SHORT( IDM_FLOAT, TRUE ),
                                   MPFROMSHORT( MIA_CHECKED ));
    if ( usState == MIA_CHECKED ) {
        fl = 0;
        fCheck = FALSE;
    }
    else {
        fl = WS_TOPMOST;
        fCheck = TRUE;
    }
    WinCheckMenuItem( global.hwndMenu, IDM_FLOAT, fCheck );
    WinSetWindowBits( global.hwndFrame, QWL_STYLE, fl, WS_TOPMOST );
}


/* ------------------------------------------------------------------------- *
 * SizeWindow                                                                *
 *                                                                           *
 * Set the size of the window and its various controls.  (The window is not  *
 * directly resizable but we set the size dynamically based on the font.)    *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd : Our window handle.                                          *
 *   POINTL ptl: Window position to set (or -1,-1 to set default position).  *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SizeWindow( HWND hwnd, POINTL ptl )
{
    FONTMETRICS fm;
    HPS         hps;
    ULONG       cxDesktop,      // width of desktop
                cxBorder,       // width of window border
                cyBorder,       // height of window border
                cxWin,          // width of our window
                cyWin,          // height of our window
                cxCtrl,         // width of current control
                xPos;           // position of current control

    cxDesktop = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
    cxBorder = 2;
    cyBorder = 2;

    hps = WinGetPS( hwnd );
    GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm );
    WinReleasePS( hps );

    cyWin  = fm.lMaxBaselineExt + 10;
    cxCtrl = ( 2 * fm.lEmInc ) + 4;
    xPos = 0;
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_INPUT ), HWND_TOP,
                     xPos, 0, cxCtrl, cyWin, SWP_SIZE | SWP_MOVE );
    xPos += cxCtrl;
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_KANJI ), HWND_TOP,
                     xPos, 0, cxCtrl, cyWin, SWP_SIZE | SWP_MOVE );
    xPos += cxCtrl + 5;
    cxCtrl = 10 * fm.lEmInc;
#ifdef TESTHOOK
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_TESTINPUT ), HWND_TOP,
                     xPos, 2, cxCtrl, cyWin - 4, SWP_SIZE | SWP_MOVE );
    xPos += 5 + cxCtrl;
#endif
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_STATUS ), HWND_TOP,
                     xPos, cyBorder, cxCtrl, cyWin - (2 * cyBorder), SWP_SIZE | SWP_MOVE );

    cxWin = xPos + cxCtrl + ( 2 * cxBorder );

    if ( ptl.x < 0 ) ptl.x = cxDesktop - cxWin;
    if ( ptl.y < 0 ) ptl.y = 0;
    WinSetWindowPos( global.hwndFrame, 0, ptl.x, ptl.y,
                     cxWin, cyWin, SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ACTIVATE );
}


/* ------------------------------------------------------------------------- *
 * SetupWindow                                                               *
 *                                                                           *
 * Perform initial setup of our program window.                              *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SetupWindow( HWND hwnd )
{
    ULONG flBtn = WS_VISIBLE | BS_PUSHBUTTON | BS_NOPOINTERFOCUS | BS_USERBUTTON;

    WinCreateWindow( hwnd, WC_BUTTON, "I", flBtn, 0, 0, 0, 0,
                     hwnd, HWND_TOP, IDD_INPUT, NULL, NULL );
    WinCreateWindow( hwnd, WC_BUTTON, "C", flBtn, 0, 0, 0, 0,
                     hwnd, HWND_TOP, IDD_KANJI, NULL, NULL );
#ifdef TESTHOOK
    WinCreateWindow( hwnd, WC_MLE, "", WS_VISIBLE,
                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_TESTINPUT, NULL, NULL );
#endif
    WinCreateWindow( hwnd, WC_STATIC, "FreeWnn", WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_STATUS, NULL, NULL );

    pfnBtnProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_INPUT), (PFNWP) ButtonProc );
    pfnBtnProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_KANJI), (PFNWP) ButtonProc );
    pfnTxtProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_STATUS), (PFNWP) StaticTextProc );

    if ( ! DosAllocSharedMem( (PVOID*) &(global.pRclConv), NULL, sizeof( RECTL ), fALLOCSHR ))
        global.pRclConv = NULL;

    SetTopmost( global.hwndFrame );
}


/* ------------------------------------------------------------------------- *
 * Set the status text to show the current mode.                             *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void UpdateStatus( HWND hwnd )
{
    CHAR szText[ MAX_STATUSZ ] = {0};
//    CHAR szBtn[ 3 ] = {0};
    USHORT usLang = pShared->fsMode & 0xF00,
           usMode = pShared->fsMode & 0xFF;

    switch ( usLang ) {
        case MODE_JP:
            switch ( usMode ) {
                case MODE_HIRAGANA:
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
//                    szBtn[ 0 ] = 0x82;
//                    szBtn[ 1 ] = 0xA0;
                    strcpy( szText,
                            pShared->fsMode & MODE_CJK? "Hiragana - Kanji": "Hiragana");
                    break;
                case MODE_KATAKANA:
//                    szBtn[ 0 ] = 0x83;
//                    szBtn[ 1 ] = 0x41;
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
                    strcpy( szText,
                            pShared->fsMode & MODE_CJK? "Katakana - Kanji": "Katakana");
                    break;
                case MODE_HALFWIDTH:
//                    szBtn[ 0 ] = 0xB1;
//                    szBtn[ 1 ] = '_';
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
                    strcpy( szText,
                            pShared->fsMode & MODE_CJK? "Halfwidth - Kanji": "Halfwidth Katakana");
                    break;
                case MODE_FULLWIDTH:
//                    szBtn[ 0 ] = 0x82;
//                    szBtn[ 1 ] = 0x60;
                    // Fullwidth mode doesn't support CJK
                    WinEnableControl( hwnd, IDD_KANJI, FALSE );
                    strcpy( szText, "Fullwidth ASCII");
                    break;
                case MODE_NONE:
//                    szBtn[ 0 ] = 'A';
//                    szBtn[ 1 ] = '_';
                    // Neither does None, obviously
                    WinEnableControl( hwnd, IDD_KANJI, FALSE );
                    strcpy( szText, "No conversion");
                    break;
            }
            break;
    }
//    szBtn[ 2 ] = 0;
//    WinSetDlgItemText( hwnd, IDD_INPUT, szBtn );
    WinSetDlgItemText( hwnd, IDD_STATUS, szText );
}


/* ------------------------------------------------------------------------- *
 * ToggleKanjiConversion                                                     *
 *                                                                           *
 * Toggle kanji (CJK) conversion on or off.                                  *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void ToggleKanjiConversion( HWND hwnd )
{
//    CHAR szBtn[ 3 ] = {0};

    if ( pShared->fsMode & MODE_CJK ) {
        pShared->fsMode &= ~MODE_CJK;
//        szBtn[ 0 ] = 0x82;
//        szBtn[ 1 ] = 0x6A;
    }
    else {
        pShared->fsMode |= MODE_CJK;
//        szBtn[ 0 ] = 0x8A;
//        szBtn[ 1 ] = 0xBF;
    }
//    szBtn[ 2 ] = 0;
//    WinSetDlgItemText( hwnd, IDD_KANJI, szBtn );

    UpdateStatus( hwnd );
}


/* ------------------------------------------------------------------------- *
 * ToggleInputConversion                                                     *
 *                                                                           *
 * Toggle input (phonetic) conversion on or off.                             *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void ToggleInputConversion( HWND hwnd )
{
    BYTE bInputMode = pShared->fsMode & 0xFF;

    if ( bInputMode ) {
        global.fsLastMode = pShared->fsMode;
        pShared->fsMode &= 0xFF00;              // turn off all
    }
    else
        pShared->fsMode = global.fsLastMode;    // restore last mode
    UpdateStatus( hwnd );
}


/* ------------------------------------------------------------------------- *
 * SetInputMode                                                              *
 *                                                                           *
 * Change the current (phonetic) input mode.                                 *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND   hwnd     : Our window handle.                                    *
 *   USHORT usNewMode: ID of the mode to switch to.  Must be at least 1, as  *
 *                     0 (MODE_NONE) is handled by ToggleInputConversion().  *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SetInputMode( HWND hwnd, USHORT usNewMode )
{
    USHORT i,
           usNumModes,      // number of input modes for this language
           usID;            // menu control ID
    BOOL   fCheck;

    pShared->fsMode &= 0xFF00;

    if ( IS_LANGUAGE( pShared->fsMode, MODE_JP ))
        usNumModes = 3;
    else if ( IS_LANGUAGE( pShared->fsMode, MODE_KR ))
        usNumModes = 1;
    else
        usNumModes = 1;        // other languages TBD

    if (( usNewMode < 1 ) || ( usNewMode > usNumModes ))
        usNewMode = 1;
    pShared->fsMode |= usNewMode;

    for ( i = 1; i <= usNumModes; i++ ) {
        usID = IDM_INPUT_BASE + i;
        if ( i == usNewMode )
            fCheck = TRUE;
        else
            fCheck = FALSE;
        WinCheckMenuItem( global.hwndMenu, usID, fCheck );
    }
    UpdateStatus( hwnd );
}


/* ------------------------------------------------------------------------- *
 * NextInputMode                                                             *
 *                                                                           *
 * Cycle to the next (phonetic) input mode.                                  *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void NextInputMode( HWND hwnd )
{
    USHORT usNumModes,      // number of input modes for this language
           usMode;          // new mode ID

    // Get total number of available modes for this language
    if ( IS_LANGUAGE( pShared->fsMode, MODE_JP ))
        usNumModes = 3;
    else if ( IS_LANGUAGE( pShared->fsMode, MODE_KR ))
        usNumModes = 1;
    else
        usNumModes = 0;        // other languages TBD

    // Get the current mode
    usMode = pShared->fsMode & 0xFF;

    // Now select the next mode
    usMode++;
    if ( usMode > usNumModes ) usMode = 1;
    SetInputMode( hwnd, usMode );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
void Draw3DBorder( HPS hps, RECTL rcl, BOOL fInset )
{
    POINTL ptl;

    GpiSetColor( hps, fInset? CLR_BLACK: SYSCLR_BUTTONDARK );
    ptl.x = rcl.xLeft;
    ptl.y = rcl.yBottom;
    GpiMove( hps, &ptl );
    ptl.y = rcl.yTop - 1;
    GpiLine( hps, &ptl );
    GpiMove( hps, &ptl );
    ptl.x = rcl.xRight - 1;
    GpiLine( hps, &ptl );
    GpiSetColor( hps, fInset? SYSCLR_BUTTONDARK: CLR_BLACK );
    ptl.x = rcl.xLeft;
    ptl.y = rcl.yBottom;
    GpiMove( hps, &ptl );
    ptl.x = rcl.xRight - 1;
    GpiLine( hps, &ptl );
    GpiMove( hps, &ptl );
    ptl.y = rcl.yTop - 2;
    GpiLine( hps, &ptl );
    GpiSetColor( hps, fInset? SYSCLR_BUTTONLIGHT: SYSCLR_BUTTONDARK );
    ptl.x = rcl.xLeft + 1;
    ptl.y = rcl.yBottom + 1;
    GpiMove( hps, &ptl );
    ptl.x = rcl.xRight - 2;
    GpiLine( hps, &ptl );
    GpiMove( hps, &ptl );
    ptl.y = rcl.yTop - 2;
    GpiLine( hps, &ptl );
    GpiSetColor( hps, fInset? SYSCLR_BUTTONDARK: SYSCLR_BUTTONLIGHT );
    ptl.x = rcl.xLeft + 1;
    ptl.y = rcl.yBottom + 2;
    GpiMove( hps, &ptl );
    ptl.y = rcl.yTop - 2;
    GpiLine( hps, &ptl );
    GpiMove( hps, &ptl );
    ptl.x = rcl.xRight - 2;
    GpiLine( hps, &ptl );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
void PaintIMEButton( PUSERBUTTON pBtnData )
{
    FONTMETRICS fm;
    CHAR   szText[ MAX_BTN_LABELZ ] = {0};
    ULONG  cbPP,
           cbText;
    LONG   lClr,
           lStrW, lStrH;    // string dimensions
    POINTL ptl, aptl[ TXTBOX_COUNT ];
    RECTL  rcl;

    if ( !pBtnData ) return;
    if ( !WinQueryWindowRect( pBtnData->hwnd, &rcl )) return;

    // Draw the button background and border
    cbPP = WinQueryPresParam( pBtnData->hwnd, PP_BACKGROUNDCOLOR,
                              PP_BACKGROUNDCOLORINDEX, NULL,
                              sizeof( lClr ), &lClr, QPF_ID2COLORINDEX );
    if ( cbPP )
        GpiCreateLogColorTable( pBtnData->hps, 0, LCOLF_RGB, 0, 0, NULL );
    else
        lClr = GpiQueryRGBColor( pBtnData->hps, 0, SYSCLR_BUTTONMIDDLE );

    WinFillRect( pBtnData->hps, &rcl, lClr );
    Draw3DBorder( pBtnData->hps, rcl, (BOOL)(pBtnData->fsState & BDS_HILITED) );

    // Draw the text
    WinQueryWindowText( pBtnData->hwnd, MAX_BTN_LABELZ, szText );
    cbText = strlen( szText );
    GpiQueryFontMetrics( pBtnData->hps, sizeof( FONTMETRICS ), &fm );
    GpiQueryTextBox( pBtnData->hps, cbText, szText, TXTBOX_COUNT, aptl );
    lStrW = aptl[ TXTBOX_CONCAT ].x;
    lStrH = fm.lEmHeight - 1;

    if ( pBtnData->fsState & BDS_DISABLED )
        lClr  = GpiQueryRGBColor( pBtnData->hps, 0, SYSCLR_MENUDISABLEDTEXT );
    else {
        cbPP = WinQueryPresParam( pBtnData->hwnd, PP_FOREGROUNDCOLOR,
                                  PP_FOREGROUNDCOLORINDEX, NULL,
                                  sizeof( lClr ), &lClr, QPF_ID2COLORINDEX );
        if ( !cbPP )
            lClr = GpiQueryRGBColor( pBtnData->hps, 0, SYSCLR_WINDOWTEXT );
    }
    GpiSetColor( pBtnData->hps, SYSCLR_WINDOW );
    ptl.x = ( RECTL_WIDTH( rcl ) / 2 ) - ( lStrW / 2 );
    ptl.y = ( RECTL_HEIGHT( rcl ) / 2) - ( lStrH / 2 );
    GpiCharStringPosAt( pBtnData->hps, &ptl, &rcl, CHS_CLIP | CHS_LEAVEPOS, cbText, szText, NULL );
    GpiSetColor( pBtnData->hps, lClr );
    ptl.x--;
    ptl.y++;
    GpiCharStringPosAt( pBtnData->hps, &ptl, &rcl, CHS_CLIP | CHS_LEAVEPOS, cbText, szText, NULL );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClientWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    HWND   hwndFocus;
    POINTL ptlWin,
           ptlMouse;
    SWP    swp;
    LONG   cb, lClr;
    RECTL  rcl;
    HPS    hps;

    switch ( msg ) {

        case WM_CREATE:
            global.hwndMenu = WinLoadMenu(HWND_OBJECT, 0, IDM_POPUP);
            return 0;

        case WM_BEGINDRAG:
            WinSetFocus( HWND_DESKTOP, hwnd );
            WinSendMsg( WinQueryWindow( hwnd, QW_PARENT ), WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), MPVOID );
            break;

        case WM_CONTEXTMENU:
            if ( global.hwndMenu != NULLHANDLE ) {
                WinQueryPointerPos(HWND_DESKTOP, &ptlMouse);
                WinPopupMenu(HWND_DESKTOP, hwnd, global.hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
                return 0;
            }
            break;

        case WM_COMMAND:
            switch( COMMANDMSG(&msg)->cmd ) {

                case IDD_INPUT:
                    ToggleInputConversion( hwnd );
                    if ( global.hwndLast ) WinSetFocus( HWND_DESKTOP, global.hwndLast );
                    break;

                case ID_HOTKEY_INPUT:
                    ToggleInputConversion( hwnd );
                    break;

                case ID_HOTKEY_MODE:
                    if ( ! IS_INPUT_MODE( pShared->fsMode, MODE_NONE ))
                        NextInputMode( hwnd );
                    break;

                case IDD_KANJI:
                    ToggleKanjiConversion( hwnd );
                    if ( global.hwndLast ) WinSetFocus( HWND_DESKTOP, global.hwndLast );
                    break;

                case ID_HOTKEY_KANJI:
                    ToggleKanjiConversion( hwnd );
                    break;

                case ID_HOTKEY_CONVERT:
                    DoClauseConversion( hwnd );
                    break;

                case ID_HOTKEY_ACCEPT:
                    AcceptClause( hwnd );
                    break;

                case ID_HOTKEY_CANCEL:
                    DismissConversionWindow( hwnd );
                    break;

                case ID_HOTKEY_NEXT:
                    EnterPhrase( hwnd, CWT_NEXT );
                    break;

                case ID_HOTKEY_PREV:
                    EnterPhrase( hwnd, CWT_PREV );
                    break;

                case IDM_HIRAGANA:
                    SetInputMode( hwnd, MODE_HIRAGANA );
                    break;

                case IDM_KATAKANA:
                    SetInputMode( hwnd, MODE_KATAKANA );
                    break;

                case IDM_HALFWIDTH:
                    SetInputMode( hwnd, MODE_HALFWIDTH );
                    break;

                case IDM_FULLWIDTH:
                    SetInputMode( hwnd, MODE_FULLWIDTH );
                    break;

                case IDM_FLOAT:
                    SetTopmost( hwnd );
                    return 0;

                case IDM_SETTINGS:
                    WinDlgBox( HWND_DESKTOP, hwnd, SettingsDlgProc, NULLHANDLE, DLG_SETTINGS, NULL );
                    return 0;

                case IDM_CLOSE:
                    WinSendMsg( hwnd, WM_CLOSE, 0L, 0L );
                    return 0;

                case IDM_ABOUT:
                    WinDlgBox( HWND_DESKTOP, hwnd, AboutDlgProc, NULLHANDLE, DLG_ABOUT, NULL );
                    return 0;
            }
            break;

        case WM_CONTROL:
            switch ( SHORT2FROMMP( mp1 )) {
                case BN_PAINT:
                    PaintIMEButton( (PUSERBUTTON) mp2 );
                    break;
            }
            break;


        case WM_FOCUSCHANGE:
            // If our window got focus, remember the previously-active window
            hwndFocus = (HWND) mp1;
            if (( SHORT1FROMMP( mp2 ) == TRUE ) &&
                ! WinIsChild( hwndFocus, global.hwndFrame ))
            {
                global.hwndLast = hwndFocus;
            }
            break;


        case WM_PAINT:
            hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );
            WinQueryWindowRect( hwnd, &rcl );
            cb = WinQueryPresParam( hwnd, PP_BACKGROUNDCOLOR, PP_BACKGROUNDCOLORINDEX,
                                    NULL, sizeof( lClr ), &lClr, QPF_ID2COLORINDEX );
            if ( cb )
                GpiCreateLogColorTable( hps, 0, LCOLF_RGB, 0, 0, NULL );
            else
                lClr = GpiQueryRGBColor( hps, 0, SYSCLR_DIALOGBACKGROUND );
            WinFillRect( hps, &rcl, lClr );
            Draw3DBorder( hps, rcl, FALSE );
            WinEndPaint( hps );
            return 0;

        case WM_PRESPARAMCHANGED:
            switch ( (ULONG) mp1 ) {
                case PP_FONTNAMESIZE:
                    WinQueryWindowPos( hwnd, &swp );
                    ptlWin.x = swp.x;
                    ptlWin.y = swp.y;
                    SizeWindow( hwnd, ptlWin );
                    break;
                default: break;
            }
            break;

        case WM_SIZE:
            WinQueryWindowPos( hwnd, &swp );
            ptlWin.x = swp.x;
            ptlWin.y = swp.y;
            SizeWindow( hwnd, ptlWin );
            break;


        // Custom msgs
        //
        default:
            if ( msg == pShared->wmAddChar ) {
                // Typed character
                ProcessCharacter( hwnd, pShared->hwndSource, mp1, mp2 );
                return 0;
            }
            else if (( msg == pShared->wmDelChar ) && global.hwndClause ) {
                // Backspace
                WinSendMsg( global.hwndClause, CWM_DELCHAR, MPFROMSHORT( 1 ), 0L );

                // Obviously this should cancel any in-progress character entry
                ClearInputBuffer();

                // And since the clause text changed, we reset the conversion state
                if (( global.fsClause & CLAUSE_READY ) == CLAUSE_READY )
                {
                    ClearConversion( global.pSession );
                    pShared->fsMode &= ~MODE_CJK_PHRASE;
                    global.fsClause &= ~CLAUSE_READY;
                }
                return 0;
            }
            break;

    }

    return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


/* ------------------------------------------------------------------------- *
 * Set the language setting.                                                 *
 * ------------------------------------------------------------------------- */
BOOL SetupDBCSLanguage( USHORT usLangMode )
{
    COUNTRYCODE cc = {0};
    INT rc;

    switch ( usLangMode ) {
        case MODE_JP:
            cc.country  = 81;       // Japan
            cc.codepage = 943;      // Japanese SJIS
            break;

        case MODE_KR:
            cc.country  = 82;       // Korea
            cc.codepage = 949;      // Korean KS-Code
            break;

        case MODE_CN:
            cc.country  = 86;       // China PRC
            cc.codepage = 1386;     // Chinese GBK
            break;

        case MODE_TW:
            cc.country  = 88;       // Taiwan
            cc.codepage = 950;      // Chinese Big-5
            break;
    }
    DosQueryDBCSEnv( sizeof( global.dbcs ), &cc, global.dbcs );
    global.codepage = cc.codepage;
    pShared->fsMode = usLangMode;   // no conversion, will set later

    if ( CreateUconvObject( cc.codepage, &(global.uconvOut) ) != ULS_SUCCESS ) {
        ErrorPopup( global.hwndClient, "Failed to create conversion object for selected codepage.");
        return FALSE;
    }

    rc = InitInputMethod( NULL, usLangMode );
    if ( rc ) {
        ErrorPopup( global.hwndClient, global.szEngineError );
        return FALSE;
    }

    rc = InitConversionMethod( NULL, usLangMode, (PVOID *) &(global.pSession) );
    if ( rc == CONV_CONNECT ) {
        ErrorPopup( global.hwndClient, global.szEngineError );
        FinishInputMethod();
        return FALSE;
    }

    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
VOID APIENTRY ExeTrap()
{
    WnnHookTerm();
    DosExitList( EXLST_EXIT, (PFNEXITLIST) ExeTrap );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
int main( int argc, char **argv )
{
    EXCEPTIONREGISTRATIONRECORD exRegRec;
    static PSZ clientClass = "WnnIM2";
    HMQ     hmq;
    QMSG    qmsg;
    ULONG   frameFlags = FCF_TASKLIST;
    HMODULE hm;
    POINTL  ptl;
    CHAR    szErr[ 256 ];

    LoadExceptq( &exRegRec, "I", "WnnIM-J");

    pShared = WnnGlobalData();
    ClearInputBuffer();

    global.hab = WinInitialize( 0 );
    hmq = WinCreateMsgQueue( global.hab, 0 );

    // Make sure the Unicode clipboard format is registered
    g_hSATbl = WinQuerySystemAtomTable();
    g_cfUnicode = WinAddAtom( g_hSATbl, "text/unicode");

    DosExitList( EXLST_ADD, (PFNEXITLIST) ExeTrap );    // trap exceptions to ensure hooks released

    CWinRegisterClass( global.hab );

    WinRegisterClass( global.hab, clientClass, ClientWndProc, CS_CLIPCHILDREN, 0 );
    global.hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0L, &frameFlags, clientClass,
                                           "WnnIM", 0L, 0, ID_ICON, &global.hwndClient );

    if ( !SetupDBCSLanguage( MODE_JP ))
        goto cleanup;

    SettingsInit( global.hwndClient, &ptl );
    SetupWindow( global.hwndClient );
    SizeWindow( global.hwndClient, ptl );

    // Set the initial mode settings; this is a bit complicated...
    if ( global.sDefMode > 0 )
        // Fixed startup mode selected
        SetInputMode( global.hwndClient, global.sDefMode );
    else if (( global.sDefMode < 0 ) && ( global.fsLastMode > 0 ))
        // Restore last active mode (if any)
        SetInputMode( global.hwndClient, global.fsLastMode );
    else
        // Leave input mode inactive, just update status text
        UpdateStatus( global.hwndClient );

    // Up to now fsLastMode was storing the last saved mode, including none/0.
    // From here on its job will be to remember the last _active_ (>0) mode.
    if ( global.fsLastMode < 1 )
        global.fsLastMode = 1;
    global.fsLastMode |= (pShared->fsMode & 0xFF00);

    WinCheckMenuItem( global.hwndMenu,
                      IDM_INPUT_BASE + (global.fsLastMode & 0xFF),
                      (BOOL) global.fsLastMode );

    // Now do our stuff
    DosLoadModule( szErr, sizeof(szErr), "wnnhook.dll", &hm );      // increment the DLL use counter for safety
    if ( WnnHookInit( global.hwndClient )) {
        while ( WinGetMsg( global.hab, &qmsg, NULLHANDLE, 0, 0 ))
            WinDispatchMsg( global.hab, &qmsg );
        WnnHookTerm();
    }
    if ( hm ) DosFreeModule( hm );

    FinishConversionMethod( global.pSession );
    FinishInputMethod();

    SettingsSave( global.hwndClient );

cleanup:
    if ( global.pRclConv ) DosFreeMem( global.pRclConv );

    WinDeleteAtom( g_hSATbl, g_cfUnicode );

    WinDestroyWindow( global.hwndFrame );
    WinDestroyMsgQueue( hmq );
    WinTerminate( global.hab );

    UninstallExceptq( &exRegRec );
    return 0;
}


