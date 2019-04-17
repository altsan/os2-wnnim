/****************************************************************************
 * wnnhook.h                                                                *
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

// Uncomment to restrict hook to client window
// #define TESTHOOK 1


// Flags for WNNSHARED.fsMode
// - Low byte is the input mode, this can be switched by the user
// - Bits 17-24 indicate the language mode, this is fixed at runtime
// - Bits 25-32 indicate the CJK conversion mode, this can be switched by the user

// NOTE: Low byte (input mode) flags are straight values, not bit flags.
#define IS_INPUT_MODE( fs, val ) (( fs & 0xFF ) == val )

#define MODE_NONE       0       // No conversion, IME mode off

// Japanese input modes
#define MODE_HIRAGANA   1       // Convert to hiragana
#define MODE_KATAKANA   2       // Convert to fullwidth katakana
#define MODE_FULLWIDTH  3       // Convert to fullwidth ASCII
#define MODE_HALFWIDTH  4       // Convert to halfwidth katakana

// Korean input modes
#define MODE_HANGUL     1       // Convert to hangul

// TBD Chinese modes (Pinyin, ABC, etc)?

// Language mode (these are also straight values)
#define IS_LANGUAGE( fs, val )   (( fs & 0xF00 ) == val )

#define MODE_JP         0x100   // Japanese
#define MODE_KR         0x200   // Korean
#define MODE_CN         0x300   // Simplified Chinese
#define MODE_TW         0x400   // Traditional Chinese


// Convert to Kanji/Hanzi/Hanja (these are bit flags)
// - In this mode, converted romaji are added to a clause buffer and converted
//   only when the user selects Convert.
#define MODE_CJK            0x1000  // clause conversion is enabled
#define MODE_CJK_ENTRY      0x2000  // clause entry is underway (window is visible)
#define MODE_CJK_PHRASE     0x4000  // phrase selection mode is active


// --------------------------------------------------------------------------
// TYPEDEFS
//

// Global data shared between DLL and app
typedef struct _Wnn_Global_Data {
    USHORT fsMode;                      // current input language & mode
    HWND   hwndSource;                  // HWND of window whose message was just intercepted
    USHORT usKeyMode,    fsVKMode;      // input mode hotkey (default Shift+Space)
    USHORT usKeyInput,   fsVKInput;     // input toggle hotkey (default Ctrl+Space)
    USHORT usKeyCJK,     fsVKCJK;       // CJK toggle hotkey (default Ctrl+`)
    USHORT usKeyConvert, fsVKConvert;   // CJK convert hotkey (default Space)
    USHORT usKeyAccept,  fsVKAccept;    // CJK accept hotkey (default Enter)
    USHORT usKeyNext,    fsVKNext;      // Next subclause hotkey (default Ctrl+Right)
    USHORT usKeyPrev,    fsVKPrev;      // Previous subclause hotkey (default Ctrl+Left)
    ATOM   wmAddChar;                   // custom message ID - send intercepted character
    ATOM   wmDelChar;                   // custom message ID - send backspace
} WNNSHARED, *PWNNSHARED;


// --------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//

PWNNSHARED EXPENTRY WnnGlobalData( void );
BOOL       EXPENTRY WnnHookInit( HWND );
BOOL       EXPENTRY WnnHookTerm( void );

