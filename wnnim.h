/****************************************************************************
 * wnnim.h                                                                  *
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

#define SZ_VERSION      "0.1"
#define MAX_VERSTRZ     32
#define MAX_STATUSZ     64
#define MAX_BTN_LABELZ  12
#define MAX_CHAR_BUF    7
#define MAX_KANA_BUF    4

#define SZ_DEFAULTFONT  "8.Helv"

#ifndef WS_TOPMOST
#define WS_TOPMOST      0x00200000L
#endif


// --------------------------------------------------------------------------
// MACROS
//

// useful rectangle macros
#define RECTL_HEIGHT(rcl)       (rcl.yTop - rcl.yBottom)
#define RECTL_WIDTH(rcl)        (rcl.xRight - rcl.xLeft)

#define ErrorPopup( text ) \
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, text, "Error", 0, MB_OK | MB_ERROR )


// --------------------------------------------------------------------------
// TYPES
//

typedef struct _WnnClientData {
    HWND  hwndFrame,            // our frame
          hwndClient,           // our client window
          hwndMenu,             // our context menu
          hwndLast;             // last window to have focus
    BYTE  dbcs[ 12 ];           // DBCS information vector (byte-ranges)
    ULONG codepage;
    CHAR  szRomaji[ MAX_CHAR_BUF ];
    CHAR  szKana[ MAX_KANA_BUF ];
} IMCLIENTDATA, *PIMCLIENTDATA;


// --------------------------------------------------------------------------
// GLOBALS
//

IMCLIENTDATA global = {0};      // our window's global data
PWNNSHARED   pShared;           // data shared with the dll


