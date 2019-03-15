
// Uncomment to restrict hook to client window
// #define TESTHOOK 1


// Flags for WNNSHARED.fsMode
// - Low byte is the input mode, this can be switched by the user
// - Bits 17-24 indicate the language mode, this is fixed at startup
// - Bits 25-32 indicate the CJK conversion mode, this can be switched by the user

// Conversion mode
#define MODE_NONE       0       // No conversion, IME mode off

// Input modes (Japanese)
#define MODE_HIRAGANA   1       // Convert to hiragana
#define MODE_KATAKANA   2       // Convert to fullwidth katakana
#define MODE_HALFWIDTH  3       // Convert to halfwidth katakana
#define MODE_FULLWIDTH  4       // Convert to fullwidth ASCII

// Input modes (Korean)
#define MODE_HANGUL     1

// TBD Chinese modes (Pinyin, ABC, etc)?

// Language mode
#define MODE_JP         0x100
#define MODE_KR         0x200
#define MODE_CN         0x400
#define MODE_TW         0x800

// Convert to Kanji/Hanzi/Hanja
// - In this mode, converted romaji are added to a clause buffer and converted
//   only when the user selects Convert.
#define MODE_CJK        0x1000


// Global data shared between DLL and app
typedef struct _Wnn_Global_Data {
    USHORT fsMode;              // current input language & mode
    HWND   hwndSource;          // HWND of window whose message was just intercepted
    ULONG  ulKeyMode;           // input mode hotkey (default Ctrl+Space)
    ULONG  ulKeyCJK;            // CJK mode hotkey (default Alt+`)
    ULONG  ulKeyConvert;        // CJK convert hotkey (default Space)
    ULONG  ulKeyAccept;         // CJK accept hotkey (default Enter)
    ATOM   wmAddChar;           // custom message ID - send intercepted character
// Not needed?
//    ATOM   wmCJKMode;           // custom message ID - user pressed CJK mode hotkey
//    ATOM   wmInputMode;         // custom message ID - user pressed input mode hotkey
//    ATOM   wmConvertCJK;        // custom message ID - user pressed convert hotkey
//    ATOM   wmAccept;            // custom message ID - user pressed accept hotkey
} WNNSHARED, *PWNNSHARED;


PWNNSHARED EXPENTRY WnnGlobalData( void );
BOOL       EXPENTRY WnnHookInit( HWND );
BOOL       EXPENTRY WnnHookTerm( void );

