/****************************************************************************
 * settings.h                                                               *
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


// --------------------------------------------------------------------------
// CONSTANTS
//

#define DEFAULT_GUI_FONT    "8.Helv"
#define DEFAULT_INPUT_FONT  "Times New Roman MT 30"


// Profile (INI) file entries

#define INI_FILE            "wnnim.ini"

#define PRF_APP_UI          "UI"
#define PRF_KEY_UIPOS       "Position"
#define PRF_KEY_UIFONT      "Font"
#define PRF_KEY_UIBACKCLR   "BgClr"
#define PRF_KEY_UIFORECLR   "FgClr"

#define PRF_KEY_INPUTFONT   "InputFont"
#define PRF_KEY_INPUTMODE   "InputMode"
#define PRF_KEY_STARTMODE   "StartMode"
#define PRF_KEY_INPUT       "KeyInput"
#define PRF_KEY_MODE        "KeyMode"
#define PRF_KEY_CJK         "KeyCJK"
#define PRF_KEY_CONVERT     "KeyConvert"
#define PRF_KEY_ACCEPT      "KeyAccept"
#define PRF_KEY_NEXT        "KeyNext"
#define PRF_KEY_PREV        "KeyPrev"


// --------------------------------------------------------------------------
// MACROS
//

#define LIST_ADD_STRING( hw, ctl, str ) \
            (SHORT) WinSendDlgItemMsg( hw, ctl, LM_INSERTITEM,  MPFROMSHORT( LIT_END ), MPFROMP( str ))
#define LIST_SET_ITEMDATA( hw, ctl, idx, val ) \
            WinSendDlgItemMsg( hw, ctl, LM_SETITEMHANDLE, MPFROMSHORT( idx ), MPFROMLONG( val ))
#define LIST_GET_ITEMDATA( hw, ctl, idx ) \
            (ULONG)(WinSendDlgItemMsg( hw, ctl, LM_QUERYITEMHANDLE, MPFROMSHORT( idx ), 0L ))
#define LIST_SELECT_ITEM( hw, ctl, idx ) \
            WinSendDlgItemMsg( hw, ctl, LM_SELECTITEM, MPFROMSHORT( idx ), MPFROMSHORT( TRUE ))

#define CHKBOX_ISCHECKED( hw, ctl ) \
            ((BOOL)( (USHORT) WinSendDlgItemMsg( hw, ctl, BM_QUERYCHECK, 0L, 0L ) == 1 ))


// --------------------------------------------------------------------------
// PUBLIC FUNCTIONS
//

MRESULT EXPENTRY SettingsDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void    _Optlink SettingsInit( HWND hwnd, PPOINTL pptl );
void    _Optlink SettingsSave( HWND hwnd );
BOOL    _Optlink SelectFont( HWND hwnd, PSZ pszFace, USHORT cbBuf );


