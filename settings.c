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
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
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
void  LocateProfile( PSZ pszProfile );
void  GetSelectedKey( HWND hwnd, USHORT usID, PUSHORT pusKC, PUSHORT pusVK );
void  SettingsPopulateKeyList( HWND hwnd, USHORT usID );
void  SettingsDlgPopulate( HWND hwnd );
void  SettingsUpdateKeys( HWND hwnd );



/* ------------------------------------------------------------------------- *
 * LocateProfile                                                             *
 *                                                                           *
 * Figure out where to place our INI file.  This will be in the same         *
 * directory as OS2.INI (the OS/2 user profile).                             *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     PSZ pszProfile: Character buffer to receive the INI filename.         *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void LocateProfile( PSZ pszProfile )
{
    ULONG ulRc;
    PSZ   pszUserIni,
          c;

    // Query the %USER_INI% environment variable which points to OS2.INI
    ulRc = DosScanEnv("USER_INI", &pszUserIni );
    if ( ulRc != NO_ERROR ) return;
    strncpy( pszProfile, pszUserIni, CCHMAXPATH );

    // Now change the filename portion to point to our own INI file
    if (( c = strrchr( pszProfile, '\\') + 1 ) != NULL ) {
        memset( c, 0, strlen(c) );
        strncat( pszProfile, INI_FILE, CCHMAXPATH - 1 );
    }
}


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
    HINI  hIni;                         // INI file
    CHAR  szIni[ CCHMAXPATH ]  = {0},   // full path of INI file
          szLang[ 6 ],                  // language (used as app name)
          szFont[ FACESIZE+4 ] = {0};   // font PP
    LONG  lClr,
          lVal;
    ULONG cb;


    LocateProfile( szIni );
    hIni = PrfOpenProfile( WinQueryAnchorBlock( hwnd ), szIni );

    switch ( pShared->fsMode & 0xF00 ) {
        case MODE_CN: strcpy( szLang, "zh_CN"); break;
        case MODE_TW: strcpy( szLang, "zh_TW"); break;
        case MODE_KR: strcpy( szLang, "ko_KR"); break;
        default:      strcpy( szLang, "ja_JP"); break;
    }

    // Colours (not saved)
    lClr = SYSCLR_DIALOGBACKGROUND;
    WinSetPresParam( hwnd, PP_BACKGROUNDCOLORINDEX, sizeof( lClr ), &lClr );
    lClr = SYSCLR_WINDOWTEXT;
    WinSetPresParam( hwnd, PP_FOREGROUNDCOLORINDEX, sizeof( lClr ), &lClr );

    // Font
    PrfQueryProfileString( hIni, PRF_APP_UI, PRF_KEY_UIFONT,
                           DEFAULT_GUI_FONT, &szFont, sizeof( szFont ) - 1 );
    WinSetPresParam( hwnd, PP_FONTNAMESIZE,
                     strlen( szFont ) + 1, (PVOID) szFont );

    // Position
    cb = sizeof( POINTL );
    if ( ! PrfQueryProfileData( hIni, PRF_APP_UI, PRF_KEY_UIPOS,
                                pptl, (PULONG) &cb ))
    {
        pptl->x = -1;
        pptl->y = -1;
    }

    // Startup mode
    cb = sizeof( global.sDefMode );
    if ( ! PrfQueryProfileData( hIni, szLang, PRF_KEY_STARTMODE,
                               &(global.sDefMode), &cb ))
        global.sDefMode = MODE_NONE;

    // Until we've finished initialization, fsLastMode will be used to store
    // the last input mode as read from the profile. This is a temporary duty.
    if ( global.sDefMode > 0 )
        global.fsLastMode = global.sDefMode;
    else {
        cb = sizeof( global.fsLastMode );
        if ( ! PrfQueryProfileData( hIni, szLang, PRF_KEY_INPUTMODE,
                               &(global.fsLastMode), &cb ))
            // Default to the first mode regardless of language
            global.fsLastMode = 1;
    }

    // Input font
    PrfQueryProfileString( hIni, szLang, PRF_KEY_INPUTFONT,
                           DEFAULT_INPUT_FONT, &(global.szInputFont),
                           sizeof( global.szInputFont ) - 1 );

    // Default hotkeys

    // Input conversion toggle
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_INPUT, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyInput = LOUSHORT( lVal );
        pShared->fsVKInput  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyInput = 0x20;
        pShared->fsVKInput  = KC_CTRL;
    }

    // Input mode switch
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_MODE, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyMode = LOUSHORT( lVal );
        pShared->fsVKMode  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyMode = 0x20;
        pShared->fsVKMode  = KC_SHIFT;
    }

    // CJK clause conversion toggle
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_CJK, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyCJK = LOUSHORT( lVal );
        pShared->fsVKCJK  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyCJK = 0x00;
        pShared->fsVKCJK  = KC_CTRL | KC_SHIFT;
    }

    // Convert current clause
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_CONVERT, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyConvert = LOUSHORT( lVal );
        pShared->fsVKConvert  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyConvert = 0x20;
        pShared->fsVKConvert  = 0;
    }

    // Accept current candidate
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_ACCEPT, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyAccept = LOUSHORT( lVal );
        pShared->fsVKAccept  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyAccept  = 0x0D;
        pShared->fsVKAccept   = 0;
    }

    // First/Next phrase
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_NEXT, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyNext = LOUSHORT( lVal );
        pShared->fsVKNext  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyNext    = VK_RIGHT;
        pShared->fsVKNext     = KC_CTRL | KC_VIRTUALKEY;
    }

    // Last/Previous phrase
    cb = sizeof( lVal );
    if ( PrfQueryProfileData( hIni, szLang, PRF_KEY_PREV, &lVal, &cb ) && ( cb == sizeof( lVal ))) {
        pShared->usKeyPrev = LOUSHORT( lVal );
        pShared->fsVKPrev  = HIUSHORT( lVal );
    }
    else {
        pShared->usKeyPrev    = VK_LEFT;
        pShared->fsVKPrev     = KC_CTRL | KC_VIRTUALKEY;
    }

    PrfCloseProfile( hIni );
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
    if ( sIdx == LIT_NONE ) return;

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
    LIST_SET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx, -1 );
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
    //LIST_SELECT_ITEM( hwnd, IDD_STARTUP_MODE, (SHORT)(global.sDefMode) + 1 );
    ListSelectDataItem( hwnd, IDD_STARTUP_MODE, (ULONG) global.sDefMode );

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
    SHORT sIdx;
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
                    sIdx = (USHORT) WinSendDlgItemMsg( hwnd, IDD_STARTUP_MODE,
                                                       LM_QUERYSELECTION,
                                                       MPFROMSHORT( LIT_FIRST ), 0 );
                    if ( sIdx != LIT_NONE )
                        global.sDefMode = (SHORT)(LIST_GET_ITEMDATA( hwnd, IDD_STARTUP_MODE, sIdx ));

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


/* ------------------------------------------------------------------------- *
 * SettingsSave                                                              *
 *                                                                           *
 * Saves various settings to the INI file.  Called on program exit.          *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   HWND hwnd: Main window handle.                                          *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void SettingsSave( HWND hwnd )
{
    HINI   hIni;                        // handle of INI file
    CHAR   szIni[ CCHMAXPATH ]  = {0},  // full path of INI file
           szLang[ 6 ],                 // language (used as app name)
           szFont[ FACESIZE+4 ] = {0};  // font PP
    SWP    wp;                          // window size/position (as queried)
    POINTL ptl;                         // window position (as saved)
    LONG   lData;                       // INI data item


    LocateProfile( szIni );
    hIni = PrfOpenProfile( WinQueryAnchorBlock( hwnd ), szIni );

    switch ( pShared->fsMode & 0xF00 ) {
        case MODE_CN: strcpy( szLang, "zh_CN"); break;
        case MODE_TW: strcpy( szLang, "zh_TW"); break;
        case MODE_KR: strcpy( szLang, "ko_KR"); break;
        default:      strcpy( szLang, "ja_JP"); break;
    }

    // Save the window position
    if ( WinQueryWindowPos( global.hwndFrame, &wp )) {
        ptl.x = wp.x;
        ptl.y = wp.y;
        PrfWriteProfileData( hIni, PRF_APP_UI, PRF_KEY_UIPOS, &ptl, sizeof( ptl ));
    }

    // Save the UI font
    if ( WinQueryPresParam( hwnd, PP_FONTNAMESIZE, 0, NULL,
                            sizeof( szFont ), szFont, 0 ))
        PrfWriteProfileString( hIni, PRF_APP_UI, PRF_KEY_UIFONT, szFont );

    // Save the configuration settings
    PrfWriteProfileString( hIni, szLang, PRF_KEY_INPUTFONT, global.szInputFont );

    PrfWriteProfileData( hIni, szLang, PRF_KEY_STARTMODE,
                         &(global.sDefMode), sizeof( global.sDefMode ));

    lData = pShared->fsMode & 0xFF;
    PrfWriteProfileData( hIni, szLang, PRF_KEY_INPUTMODE,
                         &lData, sizeof( lData ));

    lData = MAKELONG( pShared->usKeyMode, pShared->fsVKMode );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_MODE,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyInput, pShared->fsVKInput );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_INPUT,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyCJK, pShared->fsVKCJK );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_CJK,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyConvert, pShared->fsVKConvert );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_CONVERT,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyAccept, pShared->fsVKAccept );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_ACCEPT,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyNext, pShared->fsVKNext );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_NEXT,
                         &(lData), sizeof( lData ));

    lData = MAKELONG( pShared->usKeyPrev, pShared->fsVKPrev );
    PrfWriteProfileData( hIni, szLang, PRF_KEY_PREV,
                         &(lData), sizeof( lData ));

    PrfCloseProfile( hIni );
}

