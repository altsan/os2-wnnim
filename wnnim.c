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
#define INCL_GPI
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wnnhook.h"
#include "codepage.h"
#include "ids.h"


// --------------------------------------------------------------------------
// CONSTANTS
//

#define SZ_VERSION      "0.1"
#define MAX_VERSTRZ     32
#define MAX_STATUSZ     64

#define MAX_CHAR_BUF    7
#define MAX_KANA_BUF    4

#define SZ_DEFAULTFONT  "8.Helv"

#ifndef WS_TOPMOST
#define WS_TOPMOST      0x00200000L
#endif


// --------------------------------------------------------------------------
// TYPES
//

typedef struct _WnnClientData {
    HWND  hwndFrame,            // our frame
          hwndClient;           // our client window
    BYTE  dbcs[ 12 ];           // DBCS information vector (byte-ranges)
    CHAR  szRomaji[ MAX_CHAR_BUF ];
    CHAR  szKana[ MAX_KANA_BUF ];
} IMCLIENTDATA, *PIMCLIENTDATA;


// --------------------------------------------------------------------------
// GLOBALS
//

IMCLIENTDATA global;           // our window's global data
PWNNSHARED   pShared;          // data shared with the dll

// Subclassed window procedures
PFNWP pfnBtnProc;
PFNWP pfnTxtProc;


// ==========================================================================
// IMPLEMENTATION
// ==========================================================================

/* ------------------------------------------------------------------------- *
 * SendCharacter                                                             *
 *                                                                           *
 * Send the contents of the character buffer to the source window. Typically *
 * this will cause the character to be inserted at the current position,     *
 * but in practice it's up to the application to decide what to do with it.  *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void SendCharacter( HWND hwndSource, PSZ pszBuffer, MPARAM mp1 )
{
    USHORT i,
           usLen,
           usChar;

    usLen = strlen( pszBuffer );
    for ( i = 0; i < usLen; i++ ) {
        usChar = (USHORT) global.szKana[ i ];
        if ( IsDBCSLeadByte( global.szKana[ i ], global.dbcs ))
            usChar |= ( global.szKana[ ++i ] << 0x8 );
        WinSendMsg( hwndSource, WM_CHAR, mp1, MPFROM2SHORT( usChar, 0 ));
    }
    memset( global.szKana, 0, MAX_KANA_BUF );
}


/* ------------------------------------------------------------------------- *
 * ConvertCharacter                                                          *
 *                                                                           *
 * Convert the ASCII (romaji) input buffer into a phonetic character for the *
 * active language.  (We use the term 'kana', which is Japanese, but this    *
 * applies equally to Korean Hangul or the various Chinese inputs.)          *
 *                                                                           *
 * Note that the phonetic 'character' in question may consist of more than   *
 * one actual character value, because some kana are composites (similar to  *
 * how English has composite units like "ch" or "sh").                       *
 *                                                                           *
 * ------------------------------------------------------------------------- */
BOOL ConvertCharacter( void )
{
    // temp for testing (0x82a0 is Japanese 'A' hiragana)
    global.szKana[ 0 ] = 0x82;
    global.szKana[ 1 ] = 0xA0;
    global.szKana[ 2 ] = 0;

    memset( global.szRomaji, 0, MAX_CHAR_BUF );
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * ProcessCharacter                                                          *
 *                                                                           *
 * Process an input character (byte) from the input hook and decide how to   *
 * deal with it.  If we made it this far, then we know that IM mode is       *
 * active; here we determine whether the character is valid for the current  *
 * input mode and existing buffer contents, and process it accordingly.      *
 *                                                                           *
 * The romaji buffer can basically be in four states at any given time:      *
 *  - empty                                                                  *
 *  - complete (contains valid romaji sequence ready for conversion)         *
 *  - incomplete (contains a partial sequence which is potentially valid)    *
 *  - invalid (contains a sequence that is not nor could ever become valid)  *
 * ------------------------------------------------------------------------- */
void ProcessCharacter( HWND hwnd, HWND hwndSource, MPARAM mp1, MPARAM mp2 )
{
    UCHAR szChar[ 2 ];
    szChar[ 0 ] = (UCHAR) SHORT1FROMMP( mp2 );
    szChar[ 1 ] = 0;
    strncat( global.szRomaji, szChar, MAX_CHAR_BUF-1 );

    if ( strlen( global.szRomaji ) > 1 ) {          // temp logic for testing
    // TODO (eventual logic):
    //   - check if szRomaji contains value romaji sequence (probably by calling romkan API)
    //   - do the following if valid:

        ConvertCharacter(); // may not be necessary if romkan API has already done it??

        memset( global.szRomaji, 0, MAX_CHAR_BUF );     // clear romaji buffer

        // TODO: if CJK conversion is active, add szKana to the clause buffer; else:
        SendCharacter( hwndSource, global.szKana, mp1 );
    }
    // TODO:
    // - else if (romaji is invalid) OR (romaji state is incomplete AND length is max)
    //   - SendCharacter( hwndSource, global.szRomaji );
    //   - clear romaji buffer
    //   - clear kana buffer
    // - else return (keep current buffers and be ready for next character)

}


/* ------------------------------------------------------------------------- *
 * CentreWindow                                                              *
 *                                                                           *
 * Centres one window relative to another (or to the screen).  The window    *
 * will always be placed on top of the z-order (HWND_TOP).                   *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     HWND  hwndCentre  : the window to be centred                          *
 *     HWND  hwndRelative: the window relative to which hwndCentre will be   *
 *                         centred, or NULLHANDLE to centre on the screen    *
 *     ULONG flFlags     : additional flags for WinSetWindowPos (SWP_MOVE is *
 *                         always assumed                                    *
 *                                                                           *
 * RETURNS: N/A                                                              *
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
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    CHAR achVersion[ MAX_VERSTRZ ];

    switch ( msg ) {
        case WM_INITDLG:
            sprintf( achVersion, "Version %s", SZ_VERSION );
            WinSetDlgItemText( hwnd, IDD_VERSION, achVersion );
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
    switch( msg ) {
        // pass these messages up to the owner after converting the pointer pos
        case WM_BEGINDRAG:
        case WM_CONTEXTMENU:
            return PassStdEvent( hwnd, msg, mp1, mp2 );
/*
        case WM_PRESPARAMCHANGED:
            // also pass this one up, but we don't need to update the params
            return (MRESULT) WinPostMsg( WinQueryWindow( hwnd, QW_OWNER ), msg, mp1, mp2 );
*/
    }
    return (MRESULT) pfnTxtProc( hwnd, msg, mp1, mp2 );
}


/* ------------------------------------------------------------------------- *
 * SetTopmost                                                                *
 *                                                                           *
 * Toggle the 'always on top' (float) setting of the window.                 *
 * ------------------------------------------------------------------------- */
void SetTopmost( HWND hwnd, HWND hwndMenu )
{
    USHORT usState;
    ULONG  fl;

    usState = (USHORT) WinSendMsg( hwndMenu, MM_QUERYITEMATTR,
                                   MPFROM2SHORT( IDM_FLOAT, TRUE ),
                                   MPFROMSHORT( MIA_CHECKED ));
    if ( usState == MIA_CHECKED ) {
        fl = 0;
        WinCheckMenuItem( hwndMenu, IDM_FLOAT, FALSE );
    }
    else {
        fl = WS_TOPMOST;
        WinCheckMenuItem( hwndMenu, IDM_FLOAT, TRUE );
    }
    WinSetWindowBits( global.hwndFrame, QWL_STYLE, fl, WS_TOPMOST );
}


/* ------------------------------------------------------------------------- *
 * SizeWindow                                                                *
 *                                                                           *
 * Set the size of the window and its various controls.  (The window is not  *
 * directly resizable but we set the size dynamically based on the font.)    *
 * ------------------------------------------------------------------------- */
void SizeWindow( HWND hwnd )
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
    cxBorder  = WinQuerySysValue( HWND_DESKTOP, SV_CXDLGFRAME );
    cyBorder  = WinQuerySysValue( HWND_DESKTOP, SV_CYDLGFRAME );

    hps = WinGetPS( hwnd );
    GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm );
    WinReleasePS( hps );

    cyWin  = 2*fm.lMaxBaselineExt + 8;
    cxCtrl = ( 2 * fm.lEmInc );
    xPos = 0;
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_MODE ), HWND_TOP,
                     xPos, 0, cxCtrl, cyWin, SWP_SIZE | SWP_MOVE );
    xPos += cxCtrl + 1;
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_KANJI ), HWND_TOP,
                     xPos, 0, cxCtrl, cyWin, SWP_SIZE | SWP_MOVE );
    xPos += cxCtrl + 5;
    cxCtrl = 16 * fm.lEmInc;
#ifdef TESTHOOK
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_TESTINPUT ), HWND_TOP,
                     xPos, 2, cxCtrl, cyWin - 4, SWP_SIZE | SWP_MOVE );
    xPos += 5 + cxCtrl;
#endif
    WinSetWindowPos( WinWindowFromID( hwnd, IDD_STATUS ), HWND_TOP,
                     xPos, 0, cxCtrl, cyWin, SWP_SIZE | SWP_MOVE );

    cxWin = xPos + cxCtrl + ( 2 * cxBorder );
    cyWin += cyBorder * 2;
    WinSetWindowPos( global.hwndFrame, 0, cxDesktop - cxWin, 0,
                     cxWin, cyWin, SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ACTIVATE );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
void SetupWindow( HWND hwnd )
{
    LONG lClr;

    lClr = SYSCLR_DIALOGBACKGROUND;
    WinSetPresParam( hwnd, PP_BACKGROUNDCOLORINDEX, sizeof( lClr ), &lClr );
    lClr = SYSCLR_WINDOWTEXT;
    WinSetPresParam( hwnd, PP_FOREGROUNDCOLORINDEX, sizeof( lClr ), &lClr );

    WinSetPresParam( hwnd, PP_FONTNAMESIZE,
                     strlen(SZ_DEFAULTFONT)+1, (PVOID) SZ_DEFAULTFONT );

    WinCreateWindow( hwnd, WC_BUTTON, "M", WS_VISIBLE | BS_PUSHBUTTON | BS_NOPOINTERFOCUS,
                     0, 0, 0, 0, hwnd, HWND_TOP, IDD_MODE, NULL, NULL );
    WinCreateWindow( hwnd, WC_BUTTON, "K", WS_VISIBLE | BS_PUSHBUTTON | BS_NOPOINTERFOCUS,
                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_KANJI, NULL, NULL );
#ifdef TESTHOOK
//    WinCreateWindow( hwnd, WC_ENTRYFIELD, "", WS_VISIBLE | ES_AUTOSCROLL,
//                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_TESTINPUT, NULL, NULL );
    WinCreateWindow( hwnd, WC_MLE, "", WS_VISIBLE,
                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_TESTINPUT, NULL, NULL );
#endif
    WinCreateWindow( hwnd, WC_STATIC, "FreeWnn", WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER,
                     0, 0, 0, 0,  hwnd, HWND_TOP, IDD_STATUS, NULL, NULL );

    pfnBtnProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_MODE), (PFNWP) ButtonProc );
    pfnBtnProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_KANJI), (PFNWP) ButtonProc );
    pfnTxtProc = WinSubclassWindow( WinWindowFromID(hwnd, IDD_STATUS), (PFNWP) StaticTextProc );

    SizeWindow( hwnd );
}


/* ------------------------------------------------------------------------- *
 * Set the status text to show the current mode.                             *
 * ------------------------------------------------------------------------- */
void UpdateStatus( HWND hwnd )
{
    CHAR szText[ MAX_STATUSZ ] = {0};
    USHORT usLang = pShared->fsMode & 0xF00,
           usMode = pShared->fsMode & 0xFF;

    switch ( usLang ) {
        case MODE_JP:
            switch ( usMode ) {
                case MODE_HIRAGANA:
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
                    sprintf( szText,
                             pShared->fsMode & MODE_CJK? "Hiragana - Kanji": "Hiragana");
                    break;
                case MODE_KATAKANA:
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
                    sprintf( szText,
                             pShared->fsMode & MODE_CJK? "Katagana - Kanji": "Katakana");
                    break;
                case MODE_HALFWIDTH:
                    WinEnableControl( hwnd, IDD_KANJI, TRUE );
                    sprintf( szText,
                             pShared->fsMode & MODE_CJK? "Halfwidth - Kanji": "Halfwidth Katakana");
                    break;
                case MODE_FULLWIDTH:
                    // Fullwidth mode doesn't support CJK
                    WinEnableControl( hwnd, IDD_KANJI, FALSE );
                    sprintf( szText, "Fullwidth ASCII");
                    break;
                case MODE_NONE:
                    WinEnableControl( hwnd, IDD_KANJI, FALSE );
                    sprintf( szText, "No conversion");      // neither does None, obviously
                    break;
            }
            break;
    }
    WinSetDlgItemText( hwnd, IDD_STATUS, szText );
}


/* ------------------------------------------------------------------------- *
 * Toggle kanji (CJK) conversion on or off                                   *
 * ------------------------------------------------------------------------- */
void SetKanjiMode( HWND hwnd )
{
    if ( pShared->fsMode & MODE_CJK )
        pShared->fsMode &= ~MODE_CJK;
    else
        pShared->fsMode |= MODE_CJK;
    UpdateStatus( hwnd );
}


/* ------------------------------------------------------------------------- *
 * Change the current romaji conversion mode                                 *
 * ------------------------------------------------------------------------- */
void SetConversionMode( HWND hwnd )
{
    BYTE bInputMode = pShared->fsMode & 0xFF;
    // temp: for now just toggle between mode 0/1
    if ( bInputMode )
        pShared->fsMode &= 0xFF00;          // temp: turn off all
    else
        pShared->fsMode |= MODE_HIRAGANA;   // temp: turn on mode 1
    UpdateStatus( hwnd );
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY ClientWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    static HWND hwndMenu;
    POINTL ptlMouse;
    RECTL  rcl;
    HPS    hps;

    switch ( msg ) {

        case WM_CREATE:
            hwndMenu = WinLoadMenu(HWND_OBJECT, 0, IDM_POPUP);
            SetTopmost( WinQueryWindow( hwnd, QW_PARENT ), hwndMenu );
            return 0;

        case WM_BEGINDRAG:
            WinSetFocus( HWND_DESKTOP, hwnd );
            WinSendMsg( WinQueryWindow( hwnd, QW_PARENT ), WM_TRACKFRAME, MPFROMSHORT(TF_MOVE), MPVOID );
            break;

        case WM_CONTEXTMENU:
            if ( hwndMenu != NULLHANDLE ) {
                WinQueryPointerPos(HWND_DESKTOP, &ptlMouse);
                WinPopupMenu(HWND_DESKTOP, hwnd, hwndMenu, ptlMouse.x, ptlMouse.y, 0,
                                PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
                return 0;
            }
            break;

        case WM_COMMAND:
            switch( COMMANDMSG(&msg)->cmd ) {

                case IDD_MODE:
                    SetConversionMode( hwnd );
                    break;

                case IDD_KANJI:
                    SetKanjiMode( hwnd );
                    break;

                case IDM_FLOAT:
                    SetTopmost( hwnd, hwndMenu );
                    return 0;

                case IDM_CLOSE:
                    WinSendMsg( hwnd, WM_CLOSE, 0L, 0L );
                    return 0;

                case IDM_ABOUT:
                    WinDlgBox(HWND_DESKTOP, hwnd, AboutDlgProc, NULLHANDLE, DLG_ABOUT, NULL);
                    return 0;
            }
            break;

        case WM_PAINT:
            hps = WinBeginPaint( hwnd, NULLHANDLE, &rcl );
            WinQueryWindowRect( hwnd, &rcl );
            WinFillRect( hps, &rcl, CLR_PALEGRAY );
            WinEndPaint( hps );
            return 0;

        case WM_PRESPARAMCHANGED:
            switch ( (ULONG) mp1 ) {
                case PP_FONTNAMESIZE:
                    SizeWindow( hwnd );
                    //WinInvalidateRect( hwnd, NULL, FALSE );
                    break;
                default: break;
            }
            break;

        case WM_SIZE:
            SizeWindow( hwnd );
            break;

        default:                    // if this is our custom msg
            if ( msg == pShared->wmAddChar ) {
                ProcessCharacter( hwnd, pShared->hwndSource, mp1, mp2 );
                //WinSendMsg( pShared->hwndSource, WM_CHAR, mp1, MPFROM2SHORT( SHORT1FROMMP( mp2 ) & ~0x20, 0 ));
                return 0;
            }
            break;

    }

    return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


/* ------------------------------------------------------------------------- *
 * Set the language and conversion mode                                      *
 * ------------------------------------------------------------------------- */
void SetupDBCSLanguage( USHORT usLangMode )
{
    COUNTRYCODE cc = {0};

    switch ( usLangMode ) {
        case MODE_JP:
            cc.country  = 81;       // Japan
            cc.codepage = 932;      // Japanese SJIS
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

    pShared->fsMode = usLangMode;   // no conversion, will set later
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
    static PSZ clientClass = "FreeWnn2";
    HAB   hab;
    HMQ   hmq;
    QMSG  qmsg;
    ULONG frameFlags = FCF_DLGBORDER | FCF_TASKLIST;
    HMODULE hm;
    CHAR    szErr[ 256 ];

    memset( global.szRomaji, 0, MAX_CHAR_BUF );
    memset( global.szKana, 0, MAX_KANA_BUF );

    pShared = WnnGlobalData();

    hab = WinInitialize( 0 );
    hmq = WinCreateMsgQueue( hab, 0 );

    DosExitList( EXLST_ADD, (PFNEXITLIST) ExeTrap );    // trap exceptions to ensure hooks released

    WinRegisterClass( hab, clientClass, ClientWndProc, CS_CLIPCHILDREN, 0 );
    global.hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0L, &frameFlags, clientClass,
                                           "FreeWnnIME", 0L, 0, ID_ICON, &global.hwndClient );
    SetupWindow( global.hwndClient );

    SetupDBCSLanguage( MODE_JP );                       // for now
    SetConversionMode( global.hwndClient );

    // Now do our stuff
//    DosLoadModule( szErr, sizeof(szErr), "wnnhook.dll", &hm );     // increment the DLL use counter for safety
    if ( WnnHookInit( global.hwndClient )) {
        while ( WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ))
            WinDispatchMsg( hab, &qmsg );
        WnnHookTerm();
    }
//    if ( hm ) DosFreeModule( hm );

    WinDestroyWindow( global.hwndFrame );
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

    return 0;
}


