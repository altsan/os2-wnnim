
// Uncomment to restrict hook to client window
// #define TESTHOOK 1


// Flags for WNNSHARED.fsMode
// - Low byte is the input mode, this can be switched by the user
// - Bits 17-24 indicate the language mode, this is fixed at runtime
// - Bits 25-32 indicate the CJK conversion mode, this can be switched by the user

// Conversion mode
#define MODE_NONE       0       // No conversion, IME mode off

// Input modes (Japanese)
#define MODE_HIRAGANA   1       // Convert to hiragana
#define MODE_KATAKANA   2       // Convert to fullwidth katakana
#define MODE_HALFWIDTH  3       // Convert to halfwidth katakana
#define MODE_FULLWIDTH  4       // Convert to fullwidth ASCII

// Input modes (Korean)
#define MODE_HANGUL     1       // Convert to hangul

// TBD Chinese modes (Pinyin, ABC, etc)?

// Language mode
#define MODE_JP         0x100   // Japanese
#define MODE_KR         0x200   // Korean
#define MODE_CN         0x400   // Simplified Chinese
#define MODE_TW         0x800   // Traditional Chinese

// Convert to Kanji/Hanzi/Hanja
// - In this mode, converted romaji are added to a clause buffer and converted
//   only when the user selects Convert.
#define MODE_CJK        0x1000


// Global data shared between DLL and app
typedef struct _Wnn_Global_Data {
    USHORT fsMode;                      // current input language & mode
    HWND   hwndSource;                  // HWND of window whose message was just intercepted
    USHORT usKeyMode,    fsVKMode;      // input mode hotkey (default Ctrl+Space)
    USHORT usKeyCJK,     fsVKCJK;       // CJK mode hotkey (default Alt+`)
    USHORT usKeyConvert, fsVKConvert;   // CJK convert hotkey (default Space)
    USHORT usKeyAccept,  fsVKAccept;    // CJK accept hotkey (default Enter)
    ATOM   wmAddChar;                   // custom message ID - send intercepted character
    ATOM   wmDelChar;                   // custom message ID - send backspace
} WNNSHARED, *PWNNSHARED;


PWNNSHARED EXPENTRY WnnGlobalData( void );
BOOL       EXPENTRY WnnHookInit( HWND );
BOOL       EXPENTRY WnnHookTerm( void );

