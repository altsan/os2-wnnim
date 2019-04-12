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

#define SZ_VERSION      "0.2.0"
#define SZ_COPYRIGHT    "2019"
#define MAX_VERSTRZ     32
#define MAX_STATUSZ     64
#define MAX_BTN_LABELZ  12
#define MAX_CHAR_BUFZ   9
#define MAX_KANA_BUFZ   32
#define MAX_ENGINE_ERRZ 128
#define CLAUSE_INCZ     32

// Status returned by ConvertPhonetic()
#define KANA_INVALID   -1   // Phonetic conversion failed
#define KANA_PENDING    0   // Phonetic sequence is incomplete but potentially valid
#define KANA_COMPLETE   1   // Phonetic sequence is valid and complete
#define KANA_CANDIDATE  2   // Phonetic sequence is valid as-is but may still be modifiable

// Status returned by various clause conversion functions
#define CONV_FAILED    -1   // Operation failed (check error message from Wnn)
#define CONV_CONNECT   -2   // Operation not attempted due to no connection to server
#define CONV_OK         1   // Operation succeeded


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

// Global client data which does not have to be shared with wnnhook.dll.
typedef struct _WnnClientData {
    // Window handles
    HWND        hwndFrame,              // our frame
                hwndClient,             // our client window
                hwndMenu,               // our context menu
                hwndLast,               // last window to have focus
                hwndInput,              // latest window to be the conversion target
                hwndClause,             // clause conversion overlay window
                hwndCandidates;         // conversion candidate menu (TODO)

    // User options
    SHORT       sDefMode;               // default input mode (MODE_* or 0xFF for last used)

    // General state variables
    BYTE        dbcs[ 12 ];             // DBCS information vector (byte-ranges)
    ULONG       codepage;               // DBCS output codepage
    USHORT      fsLastMode;             // last active input mode
    PRECTL      pRclConv;               // position from WM_QUERYCONVERTPOS

    // Current conversion data
    CHAR        szRomaji[ MAX_CHAR_BUFZ ];         // input buffer (characters as typed)
    UniChar     uszKana[ MAX_KANA_BUFZ ];          // buffer for converted phonetic characters
    UniChar     uszPending[ MAX_KANA_BUFZ ];       // buffer for 'candidate' phonetic characters (Korean only)
    UconvObject uconvOut;                          // conversion object for DBCS output codepage
    CHAR        szEngineError[ MAX_ENGINE_ERRZ ];  // error messages from the IME engine
    PVOID       pSession;                          // IME engine session object

} IMCLIENTDATA, *PIMCLIENTDATA;


// --------------------------------------------------------------------------
// GLOBALS
//

PWNNSHARED   pShared;           // data shared with the dll


// --------------------------------------------------------------------------
// COMMON FUNCTIONS
//

void CentreWindow( HWND hwndCentre, HWND hwndRelative, ULONG flFlags );

