/****************************************************************************
 * settings.c                                                               *
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
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uconv.h>

#include "ids.h"
#include "wnnhook.h"
#include "wnnim.h"
#include "settings.h"


extern IMCLIENTDATA global;

BOOL  ListSelectDataItem( HWND hwnd, USHORT usID, ULONG ulHandle );
void  GetSelectedKey( HWND hwnd, USHORT usID, PUSHORT pusKC, PUSHORT pusVK );
void  SettingsPopulateKeyList( HWND hwnd, USHORT usID );
void  SettingsDlgPopulate( HWND hwnd );
void  SettingsUpdateKeys( HWND hwnd );



/* ------------------------------------------------------------------------- *
 * SettingsInit                                                              *
 *                                                                           *
 * Set the initial program settings.                                         *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND    hwnd: Our window handle.                                        *
 *   PPOINTL pptl: Pointer to window position.                               *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void _Optlink SettingsInit( HWND hwnd, PPOINTL pptl )
{
    LONG  lClr;

    // Colours
    lClr = SYSCLR_DIALOGBACKGROUND;
    WinSetPresParam( hwnd, PP_BACKGROUNDCOLORINDEX, sizeof( lClr ), &lClr );
    lClr = SYSCLR_WINDOWTEXT;
    WinSetPresParam( hwnd, PP_FOREGROUNDCOLORINDEX, sizeof( lClr ), &lClr );

    // Font
    WinSetPresParam( hwnd, PP_FONTNAMESIZE,
                     strlen( DEFAULT_GUI_FONT )+1, (PVOID) DEFAULT_GUI_FONT );

    // Position
    pptl->x = -1;
    pptl->y = -1;

    // Startup mode
    global.sDefMode = -1;
    global.fsLastMode = 1;
    strncpy( global.szInputFont, DEFAULT_INPUT_FONT, FACESIZE );

    // Default hotkeys
    pShared->usKeyInput   = 0x20;
    pShared->fsVKInput    = KC_CTRL;

    pShared->usKeyMode    = 0x20;
    pShared->fsVKMode     = KC_SHIFT;

    pShared->usKeyCJK     = 0x00;
    pShared->fsVKCJK      = KC_CTRL | KC_SHIFT;

    pShared->usKeyConvert = 0x20;
    pShared->fsVKConvert  = 0;

    pShared->usKeyAccept  = 0x0D;
    pShared->fsVKAccept   = 0;

    pShared->usKeyNext    = VK_RIGHT;
    pShared->fsVKNext     = KC_CTRL | KC_VIRTUALKEY;

    pShared->usKeyPrev    = VK_LEFT;
    pShared->fsVKPrev     = KC_CTRL | KC_VIRTUALKEY;

}


/* ------------------------------------------------------------------------- *
 * ListSelectDataItem                                                        *
 *                                                                           *
 * Select the listbox/combobox item with the given item handle value.        *
 * ------------------------------------------------------------------------- */
BOOL ListSelectDataItem( HWND hwnd, USHORT usID, ULONG ulHandle )
{
    SHORT i;
    SHORT sIdx;
    ULONG ulData;

    sIdx = (SHORT) WinSendDlgItemMsg( hwnd, usID, LM_QUERYITEMCOUNT, 0L, 0L );
    if ( !sIdx ) return FALSE;

    for ( i = 0; i < sIdx; i ++ ) {
        ulData = LIST_GET_ITEMDATA( hwnd, usID, i );
        if ( ulData == ulHandle ) break;
    }
    LIST_SELECT_ITEM( hwnd, usID, i );
    return TRUE;
}


/* ------------------------------------------------------------------------- *
 * GetSelectedKey                                                            *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void GetSelectedKey( HWND hwnd, USHORT usID, PUSHORT pusKC, PUSHORT pusVK )
{
    SHORT sIdx;
    ULONG ulData;

    sIdx = (USHORT) WinSendDlgItemMsg( hwnd, usID, LM_QUERYSELECTION,
                                       MPFROMSHORT( LIT_FIRST ), 0 );
    if ( !sIdx ) return;

    ulData = LIST_GET_ITEMDATA( hwnd, usID, sIdx );
    *pusVK |= (HIUSHORT( ulData ) & KC_VIRTUALKEY );
    *pusKC = LOUSHORT( ulData );
}


/* ------------------------------------------------------------------------- *
 * SettingsPopulateKeyList                                                   *
 *                                                                           *
 * Populate a key selection list with our standard set of supported keys.    *
 * Each item's handle (item data) has the key character code or virtual key  *
 * code in the low USHORT; the high USHORT is KC_VIRTUALKEY in the case of a *
 * virtual key or 0 otherwise.                                               *
 * ------------------------------------------------------------------------- */
void SettingsPopulateKeyList( HWND hwnd, USHORT usID )
{
    SHORT sIdx;
    sIdx = LIST_ADD_STRING( hwnd, usID, "Space");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, 0x20 );
    sIdx = LIST_ADD_STRING( hwnd, usID, "Enter");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, 0x0D );
    sIdx = LIST_ADD_STRING( hwnd, usID, "`");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, (ULONG)'`');
    sIdx = LIST_ADD_STRING( hwnd, usID, ".");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, (ULONG)'.');
    sIdx = LIST_ADD_STRING( hwnd, usID, ",");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, (ULONG)',');
    sIdx = LIST_ADD_STRING( hwnd, usID, "/");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, (ULONG)'/');
    sIdx = LIST_ADD_STRING( hwnd, usID, "Left");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_LEFT, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "Right");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_RIGHT, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "Up");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_UP, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "Down");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_DOWN, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F1");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F1, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F2");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F2, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F3");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F3, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F4");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F4, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F5");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F5, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F6");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F6, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F7");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F7, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F8");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F8, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F9");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F9, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F10");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F10, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F11");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F11, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "F12");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, MAKEULONG( VK_F12, KC_VIRTUALKEY ));
    sIdx = LIST_ADD_STRING( hwnd, usID, "");
    LIST_SET_ITEMDATA( hwnd, usID, sIdx, 0 );
}


/* ------------------------------------------------------------------------- *
 * SettingsDlgPopulate                                                       *
 *                                                                           *
 * Populate the controls in the settings dialog and select the current       *
 * configuration values.                                                     *
 * ------------------------------------------------------------------------- */
void SettingsDlgPopulate( HWND hwnd )
{
    SHORT sIdx;

    sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "Remember last used");
    sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "None");
    LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, MODE_NONE );

    if ( IS_LANGUAGE( pShared->fsMode, MODE_JP )) {
        sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "Hiragana");
        LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, MODE_HIRAGANA );
        sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "Katakana");
        LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, MODE_KATAKANA );
        sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "Fullwidth ASCII");
        LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, MODE_FULLWIDTH );
    }
    else if ( IS_LANGUAGE( pShared->fsMode, MODE_KR )) {
        sIdx = LIST_ADD_STRING( hwnd, IDD_STARTUP_MODE, "Hangul");
        LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, MODE_HANGUL );
    }
    LIST_SELECT_ITEM( hwnd, IDD_STARTUP_MODE, (SHORT)(global.sDefMode) + 1 );

    SettingsPopulateKeyList( hwnd, IDD_INPUT_KEY );
    ListSelectDataItem( hwnd, IDD_INPUT_KEY, MAKEULONG( pShared->usKeyInput, pShared->fsVKInput & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_INPUT_CTRL, (pShared->fsVKInput & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_INPUT_SHIFT, (pShared->fsVKInput & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_MODE_KEY );
    ListSelectDataItem( hwnd, IDD_MODE_KEY, MAKEULONG( pShared->usKeyMode, pShared->fsVKMode & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_MODE_CTRL, (pShared->fsVKMode & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_MODE_SHIFT, (pShared->fsVKMode & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_CLAUSE_KEY );
    ListSelectDataItem( hwnd, IDD_CLAUSE_KEY, MAKEULONG( pShared->usKeyCJK, pShared->fsVKCJK & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_CLAUSE_CTRL, (pShared->fsVKCJK & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_CLAUSE_SHIFT, (pShared->fsVKCJK & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_CONVERT_KEY );
    ListSelectDataItem( hwnd, IDD_CONVERT_KEY, MAKEULONG( pShared->usKeyConvert, pShared->fsVKConvert & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_CONVERT_CTRL, (pShared->fsVKConvert & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_CONVERT_SHIFT, (pShared->fsVKConvert & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_ACCEPT_KEY );
    ListSelectDataItem( hwnd, IDD_ACCEPT_KEY, MAKEULONG( pShared->usKeyAccept, pShared->fsVKAccept & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_ACCEPT_CTRL, (pShared->fsVKAccept & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_ACCEPT_SHIFT, (pShared->fsVKAccept & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_NEXT_KEY );
    ListSelectDataItem( hwnd, IDD_NEXT_KEY, MAKEULONG( pShared->usKeyNext, pShared->fsVKNext & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_NEXT_CTRL, (pShared->fsVKNext & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_NEXT_SHIFT, (pShared->fsVKNext & KC_SHIFT)? TRUE: FALSE );

    SettingsPopulateKeyList( hwnd, IDD_PREV_KEY );
    ListSelectDataItem( hwnd, IDD_PREV_KEY, MAKEULONG( pShared->usKeyPrev, pShared->fsVKPrev & KC_VIRTUALKEY ));
    WinCheckButton( hwnd, IDD_PREV_CTRL, (pShared->fsVKPrev & KC_CTRL)? TRUE: FALSE );
    WinCheckButton( hwnd, IDD_PREV_SHIFT, (pShared->fsVKPrev & KC_SHIFT)? TRUE: FALSE );
}


/* ------------------------------------------------------------------------- *
 * SettingsUpdateKeys                                                        *
 *                                                                           *
 * Update the active program hotkey settings based on the dialog contents.   *
 * ------------------------------------------------------------------------- */
void SettingsUpdateKeys( HWND hwnd )
{
    pShared->fsVKInput = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_INPUT_CTRL ))
        pShared->fsVKInput |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_INPUT_SHIFT ))
        pShared->fsVKInput |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_INPUT_KEY, &(pShared->usKeyInput), &(pShared->fsVKInput) );

    pShared->fsVKMode = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_MODE_CTRL ))
        pShared->fsVKMode |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_MODE_SHIFT ))
        pShared->fsVKMode |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_MODE_KEY, &(pShared->usKeyMode), &(pShared->fsVKMode) );

    pShared->fsVKCJK = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_CLAUSE_CTRL ))
        pShared->fsVKCJK |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_CLAUSE_SHIFT ))
        pShared->fsVKCJK |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_CLAUSE_KEY, &(pShared->usKeyCJK), &(pShared->fsVKCJK) );

    pShared->fsVKConvert = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_CONVERT_CTRL ))
        pShared->fsVKConvert |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_CONVERT_SHIFT ))
        pShared->fsVKConvert |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_CONVERT_KEY, &(pShared->usKeyConvert), &(pShared->fsVKConvert) );

    pShared->fsVKAccept = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_ACCEPT_CTRL ))
        pShared->fsVKAccept |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_ACCEPT_SHIFT ))
        pShared->fsVKAccept |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_ACCEPT_KEY, &(pShared->usKeyAccept), &(pShared->fsVKAccept) );

    pShared->fsVKNext = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_NEXT_CTRL ))
        pShared->fsVKNext |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_NEXT_SHIFT ))
        pShared->fsVKNext |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_NEXT_KEY, &(pShared->usKeyNext), &(pShared->fsVKNext) );

    pShared->fsVKPrev = 0;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_PREV_CTRL ))
        pShared->fsVKPrev |= KC_CTRL;
    if ( CHKBOX_ISCHECKED( hwnd, IDD_PREV_SHIFT ))
        pShared->fsVKPrev |= KC_SHIFT;
    GetSelectedKey( hwnd, IDD_PREV_KEY, &(pShared->usKeyPrev), &(pShared->fsVKPrev) );

}


/* ------------------------------------------------------------------------- *
 * Window procedure for settings dialog.                                     *
 * See OS/2 PM reference for a description of input and output.              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY SettingsDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    UCHAR szFontPP[ FACESIZE + 4 ];
    PSZ   psz;

    switch ( msg ) {
        case WM_INITDLG:
            SettingsDlgPopulate( hwnd );
            if ( global.szInputFont[0] ) {
                sprintf( szFontPP, "10.%s", global.szInputFont );
                WinSetPresParam( WinWindowFromID( hwnd, IDD_INPUT_FONT ),
                                 PP_FONTNAMESIZE, strlen(szFontPP)+1, szFontPP );
                psz = strchr( szFontPP, '.') + 1;
                WinSetDlgItemText( hwnd, IDD_INPUT_FONT, psz );
            }
            CentreWindow( hwnd, NULLHANDLE, SWP_SHOW );
            break;

        case WM_COMMAND:
            switch ( SHORT1FROMMP( mp1 )) {
                case IDD_FONT_SELECT:
                    if ( ! WinQueryPresParam( WinWindowFromID( hwnd, IDD_INPUT_FONT ),
                                              PP_FONTNAMESIZE, 0, NULL,
                                              sizeof(szFontPP), szFontPP, QPF_NOINHERIT ))
                        sprintf( szFontPP, "10.%.31s", DEFAULT_INPUT_FONT );
                    psz = strchr( szFontPP, '.') + 1;
                    if ( SelectFont( hwnd, psz,
                                     sizeof(szFontPP) - ( strlen(szFontPP) - strlen(psz) )))
                    {
                        WinSetPresParam( WinWindowFromID( hwnd, IDD_INPUT_FONT ),
                                         PP_FONTNAMESIZE, strlen(szFontPP)+1, szFontPP );
                        WinSetDlgItemText( hwnd, IDD_INPUT_FONT, psz );
                    }
                    else ErrorPopup("Error creating font dialog.");
                    return (MRESULT) FALSE;

                case DID_OK:
                    SettingsUpdateKeys( hwnd );

                    if ( WinQueryPresParam( WinWindowFromID( hwnd, IDD_INPUT_FONT ),
                                            PP_FONTNAMESIZE, 0, NULL,
                                            sizeof(szFontPP), szFontPP, QPF_NOINHERIT ))
                    {
                        psz = strchr( szFontPP, '.') + 1;
                        if ( !psz ) psz = szFontPP;
                        strncpy( global.szInputFont, psz, FACESIZE );
                    }

                    break;
            }
            break;

    }
    return WinDefDlgProc (hwnd, msg, mp1, mp2) ;
}


/* ------------------------------------------------------------------------- *
 * SelectFont                                                                *
 *                                                                           *
 * Pop up a font selection dialog.                                           *
 *                                                                           *
 * Parameters:                                                               *
 *   HWND   hwnd   : handle of the current window.                           *
 *   PSZ    pszFace: pointer to buffer containing current font name.         *
 *   USHORT cbBuf  : size of the buffer pointed to by 'pszFace'.             *
 *                                                                           *
 * RETURNS: BOOL                                                             *
 * ------------------------------------------------------------------------- */
BOOL _Optlink SelectFont( HWND hwnd, PSZ pszFace, USHORT cbBuf )
{
    FONTDLG     fontdlg = {0};
    FONTMETRICS fm      = {0};
    LONG        lQuery  = 0;
    CHAR        szName[ FACESIZE ];
    HWND        hwndFD;
    HPS         hps;

    hps = WinGetPS( hwnd );
    strncpy( szName, pszFace, FACESIZE-1 );

    // Get the metrics of the current font (we'll want to know the weight class)
    lQuery = 1;
    GpiQueryFonts( hps, QF_PUBLIC, pszFace, &lQuery, sizeof(fm), &fm );

    fontdlg.cbSize         = sizeof( FONTDLG );
    fontdlg.hpsScreen      = hps;
    fontdlg.pszTitle       = NULL;
    fontdlg.pszPreview     = NULL;
    fontdlg.pfnDlgProc     = NULL;
    fontdlg.pszFamilyname  = szName;
    fontdlg.usFamilyBufLen = sizeof( szName );
    fontdlg.fxPointSize    = ( fm.fsDefn & FM_DEFN_OUTLINE ) ?
                                MAKEFIXED( 10, 0 ) :
                                ( fm.sNominalPointSize / 10 );
    fontdlg.usWeight       = (USHORT) fm.usWeightClass;
    fontdlg.clrFore        = SYSCLR_WINDOWTEXT;
    fontdlg.clrBack        = SYSCLR_WINDOW;
    fontdlg.fl             = FNTS_CENTER | FNTS_CUSTOM;
    fontdlg.flStyle        = 0;
    fontdlg.flType         = ( fm.fsSelection & FM_SEL_ITALIC ) ? FTYPE_ITALIC : 0;
    fontdlg.usDlgId        = DLG_FONT;
    fontdlg.hMod           = NULLHANDLE;

    hwndFD = WinFontDlg( HWND_DESKTOP, hwnd, &fontdlg );
    WinReleasePS( hps );
    if (( hwndFD ) && ( fontdlg.lReturn == DID_OK )) {
        strncpy( pszFace, fontdlg.fAttrs.szFacename, cbBuf-1 );
        return TRUE;
    }
    return FALSE;
}

