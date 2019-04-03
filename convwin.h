// Window class name
#define WC_WNNIMCONVWIN    "WnnImConvWin"

// Custom window messages

#define CWM_QUERYTEXTLENGTH     (WM_USER + 100)   // query phrase or clause text length
#define CWM_GETTEXT             (WM_USER + 101)   // get phrase or clause text
#define CWM_SETTEXT             (WM_USER + 102)   // set phrase or clause text
#define CWM_ADDCHAR             (WM_USER + 103)   // append character to the clause
#define CWM_DELCHAR             (WM_USER + 104)   // delete last character from the clause
#define CWM_SELECTPRASE         (WM_USER + 105)   // next, prev, or none

#define CWT_ALL                 0xFFFF
#define CWT_NONE                0xFFFE
#define CWT_FIRST               0xFFFD
#define CWT_LAST                0xFFFC
#define CWT_NEXT                0xFFFB
#define CWT_PREV                0xFFFA

#define MAX_PHRASES             127     // Seems reasonable...
#define MAX_CLAUSE_LENGTH       1024    // Ditto


// Public control data for the conversion overlay window, used by WM_CREATE
typedef struct _COW_Ctl_Data {
    USHORT      cb;                       // size of this data structure
    USHORT      fsMode;                   // mode flags
} CWCTLDATA, *PCWCTLDATA;


// Internal control data for the conversion overlay window
typedef struct _COW_Private_Data {
    CWCTLDATA   ctldata;                // public control data
    USHORT      id;                     // our window ID
    ULONG       flFlags;                // internal flags (TBD)
    UniChar   * puszText;               // current control text (in UCS-2)
    USHORT      usTextLen;              // length of text (in UniChars)
    IPT         ipt;                    // current input position
    ULONG       ulNumPhrases;           // total number of sub-phrases
    ULONG       ulCurrentPhrase;        // number of the current sub-phrase
    PUSHORT     pusPhraseOffset;        // array of phrase offsets
    LONG        lDPI;                   // current font DPI
    FATTRS      fattrs;                 // attributes of the display font
} CWDATA, *PCWDATA;



BOOL CWinRegisterClass( HAB hab );

