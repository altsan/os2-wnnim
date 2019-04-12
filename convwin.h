/****************************************************************************
 * convwin.h                                                                *
 *                                                                          *
 * Clause conversion overlay window.  This is the input window for kana/    *
 * phonetic characters which are waiting to be converted to CJK ideographs. *
 * It appears as an overlay over the current input window (ideally at the   *
 * current cursor position, although this depends on the application        *
 * properly handling the WM_QUERYCONVERTPOS message).                       *
 *                                                                          *
 * The current, complete text sequence is referred to as the 'clause'.  The *
 * clause may optionally be subdivided into phrases (that is, word-phrases  *
 * in the linguistic sense, generally one or two words with particles).     *
 * By default, the whole text is considered the active conversion target;   *
 * however, a specific phrase can be made active ('selected'), meaning that *
 * it becomes the sole target for the next conversion operation.  The user  *
 * can add or delete characters (but only at the end of the clause), change *
 * the selected phrase, trigger conversion of the active phrase or clause,  *
 * or accept the current text causing it to be inserted into the current    *
 * application window.  (These last two operations must be handled by our   *
 * parent (the IME); this control window only concerns itself with managing *
 * the text contents and phrase boundaries.)                                *
 *                                                                          *
 * This window stores (and displays) all its text as UCS-2 Unicode (i.e.    *
 * UniChar strings).  This requires it to use a Unicode-capable font.       *
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

// Window class name
#define WC_WNNIMCONVWIN    "WnnImConvWin"

// Window style flags

#define CWS_BORDER              0x1

// Custom window messages

#define CWM_QUERYTEXTLENGTH     (WM_USER + 100)
#define CWM_GETTEXT             (WM_USER + 101)
#define CWM_SETTEXT             (WM_USER + 102)
#define CWM_ADDCHAR             (WM_USER + 103)
#define CWM_DELCHAR             (WM_USER + 104)
#define CWM_SETPHRASES          (WM_USER + 105)
#define CWM_SELECTPHRASE        (WM_USER + 106)
#define CWM_GETSELECTEDPHRASE   (WM_USER + 107)
#define CWM_SETINPUTWINDOW      (WM_USER + 110)

#define CWT_ALL                 0xFFFF
#define CWT_NONE                0xFFFE
#define CWT_FIRST               0xFFFD
#define CWT_LAST                0xFFFC
#define CWT_NEXT                0xFFFB
#define CWT_PREV                0xFFFA

#define MAX_PHRASES             127     // Seems reasonable...
#define MAX_CLAUSE_LENGTH       1024    // Ditto

#define BUFFER_INCREMENT        32      // Default text buffer increment (in UniChars)


#define FTYPE_NOTFOUND 0    // font does not exist
#define FTYPE_FOUND    1    // font name was found
#define FTYPE_LANGUAGE 2    // font supports requested charset
#define FTYPE_UNICODE  4    // font supports Unicode
#define FTYPE_BITMAP   8    // font is a bitmap font


// Public control data for the conversion overlay window, used by WM_CREATE
typedef struct _COW_Ctl_Data {
    USHORT      cb;                     // size of this data structure
    USHORT      fsMode;                 // mode flags
    ULONG       flFlags;                // style flags
} CWCTLDATA, *PCWCTLDATA;


// Internal control data for the conversion overlay window
typedef struct _COW_Private_Data {
    CWCTLDATA   ctldata;                // public control data
    USHORT      id;                     // our window ID
    UniChar   * puszText;               // current control text (in UCS-2)
    USHORT      usTextLen;              // length of text (in UniChars)
    USHORT      usBufLen;               // length of preallocated text buffer (in UniChars)
    USHORT      usPhraseCount;          // total number of sub-phrases
    USHORT      usCurrentPhrase;        // number of the current sub-phrase
    PUSHORT     pusPhraseEnd;           // array of phrase-end positions
    LONG        lDPI;                   // current font DPI
//    LONG        lCursorHeight;          // current cursor height
    FATTRS      fattrs;                 // attributes of the display font
    HWND        hwndSource;             // window to which we are 'attached'
} CWDATA, *PCWDATA;


// Functions

BOOL CWinRegisterClass( HAB hab );
BYTE ResolveFont( HPS hps, PSZ pszFontFace, PFATTRS pfAttrs, LONG lCell, USHORT flLang );
LONG GetCurrentDPI( HWND hwnd );

