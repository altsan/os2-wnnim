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
BOOL    _Optlink SelectFont( HWND hwnd, PSZ pszFace, USHORT cbBuf );


