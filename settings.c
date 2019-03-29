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


// --------------------------------------------------------------------------
// MACROS
//

#define LIST_ADD_STRING( hw, ctl, str ) \
            (SHORT) WinSendDlgItemMsg( hw, ctl, LM_INSERTITEM,  MPFROMSHORT( LIT_END ), MPFROMP( str ))
#define LIST_SET_ITEMDATA( hw, ctl, idx, val ) \
            WinSendDlgItemMsg( hw, ctl, LM_SETITEMHANDLE, MPFROMSHORT( idx ), MPFROMLONG( val ))


// ==========================================================================
// IMPLEMENTATION
// ==========================================================================


/* ------------------------------------------------------------------------- *
 * SettingsInit                                                              *
 *                                                                           *
 * Set the initial program settings.                                         *
 *                                                                           *
 * PARAMETERS:                                                               *
 *   HWND hwnd: Our window handle.                                           *
 *                                                                           *
 * RETURNS: n/a                                                              *
 * ------------------------------------------------------------------------- */
void SettingsInit( HWND hwnd )
{
    // Default hotkeys (should eventually be configurable)
    pShared->usKeyInput   = 0x20;
    pShared->fsVKInput    = KC_CTRL;

    pShared->usKeyMode    = 0x20;
    pShared->fsVKMode     = KC_SHIFT;

    pShared->usKeyCJK     = 0x00;
    pShared->fsVKCJK      = KC_CTRL | KC_SHIFT;

    pShared->usKeyConvert = 0x20;
    pShared->fsVKConvert  = 0;

    pShared->usKeyAccept  = 0;
    pShared->fsVKAccept   = VK_NEWLINE;
}


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
void SettingsPopulateKeyList( HWND hwnd, USHORT usID )
{
    LIST_ADD_STRING( hwnd, usID, "Space");
    LIST_ADD_STRING( hwnd, usID, "Enter");
    LIST_ADD_STRING( hwnd, usID, "`");
    LIST_ADD_STRING( hwnd, usID, ".");
    LIST_ADD_STRING( hwnd, usID, ",");
    LIST_ADD_STRING( hwnd, usID, "/");
    LIST_ADD_STRING( hwnd, usID, "Left");
    LIST_ADD_STRING( hwnd, usID, "Right");
    LIST_ADD_STRING( hwnd, usID, "Up");
    LIST_ADD_STRING( hwnd, usID, "Down");
    LIST_ADD_STRING( hwnd, usID, "F1");
    LIST_ADD_STRING( hwnd, usID, "F2");
    LIST_ADD_STRING( hwnd, usID, "F3");
    LIST_ADD_STRING( hwnd, usID, "F4");
    LIST_ADD_STRING( hwnd, usID, "F5");
    LIST_ADD_STRING( hwnd, usID, "F6");
    LIST_ADD_STRING( hwnd, usID, "F7");
    LIST_ADD_STRING( hwnd, usID, "F8");
    LIST_ADD_STRING( hwnd, usID, "F9");
    LIST_ADD_STRING( hwnd, usID, "F10");
    LIST_ADD_STRING( hwnd, usID, "F11");
    LIST_ADD_STRING( hwnd, usID, "F12");
    LIST_ADD_STRING( hwnd, usID, "(none)");
}


/* ------------------------------------------------------------------------- *
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

    WinSendDlgItemMsg( hwnd, IDD_STARTUP_MODE, LM_SELECTITEM,
                       MPFROMSHORT( 1 ), MPFROMSHORT( TRUE ));      // temp

    SettingsPopulateKeyList( hwnd, IDD_INPUT_KEY );
    SettingsPopulateKeyList( hwnd, IDD_MODE_KEY );
    SettingsPopulateKeyList( hwnd, IDD_CLAUSE_KEY );
    SettingsPopulateKeyList( hwnd, IDD_CONVERT_KEY );
    SettingsPopulateKeyList( hwnd, IDD_ACCEPT_KEY );
    SettingsPopulateKeyList( hwnd, IDD_NEXT_KEY );
    SettingsPopulateKeyList( hwnd, IDD_PREV_KEY );
}


/* ------------------------------------------------------------------------- *
 * Window procedure for settings dialog.                                     *
 * See OS/2 PM reference for a description of input and output.              *
 * ------------------------------------------------------------------------- */
MRESULT EXPENTRY SettingsDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{

    switch ( msg ) {
        case WM_INITDLG:
            SettingsDlgPopulate( hwnd );
            CentreWindow( hwnd, NULLHANDLE, SWP_SHOW );
            break;

    }
    return WinDefDlgProc (hwnd, msg, mp1, mp2) ;
}



