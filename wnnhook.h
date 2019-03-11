
// Uncomment to restrict hook to client window
// #define TESTHOOK 1


//#define USE_EXCEPTQ 1

#ifdef USE_EXCEPTQ
#define INCL_LOADEXCEPTQ
#include <exceptq.h>
#endif

#define MAX_CHAR_BUF    7
#define MAX_KANA_BUF    4


// Flags for WNNSHARED.fsMode

// Conversion mode
#define MODE_NONE       0       // No conversion, IME mode off

// Phonetic (as typed) - Japanese modes
#define MODE_HIRAGANA   1       // Convert to hiragana
#define MODE_KATAKANA   2       // Convert to fullwidth katakana
#define MODE_HALFWIDTH  3       // Convert to halfwidth katakana
#define MODE_FULLWIDTH  4       // Convert to fullwidth ASCII

// Phonetic (as typed) - Korean modes
#define MODE_HANGUL     1

// TBD Does Chinese support purely phonetic modes, e.g. non-converted Pinyin?

// Convert to Kanji/Hanzi/Hanja (build clause buffer and convert on accept command)
#define MODE_CJK        0x10

// High byte: language mode
#define MODE_JP         0x100
#define MODE_KR         0x200
#define MODE_CN         0x400
#define MODE_TW         0x800

// Global data shared between DLL and app
typedef struct _Wnn_Global_Data {
    USHORT fsMode;
    USHORT usChar;
    CHAR   szRomaji[ MAX_CHAR_BUF ];
    CHAR   szKana[ MAX_KANA_BUF ];
    BOOL   fDoubleConsonant;
    ATOM   wmAddChar;
    CHAR   szAddChar[ 40 ];
} WNNSHARED, *PWNNSHARED;


PWNNSHARED EXPENTRY WnnGlobalData( void );
BOOL       EXPENTRY WnnHookInit( HWND );
BOOL       EXPENTRY WnnHookTerm( void );

