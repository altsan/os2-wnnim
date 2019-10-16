/****************************************************************************
 * ids.h                                                                    *
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

#define ID_ICON                     1

#define IDD_INPUT                   100
#define IDD_KANJI                   101
#define IDD_STATUS                  102
#define IDD_TESTINPUT               110

#define IDD_CLAUSE                  120

#define DLG_SETTINGS                200
#define IDD_STARTUP_MODE            201
#define IDD_INPUT_FONT              202
#define IDD_FONT_SELECT             203
#define IDD_MODE_KEY                210
#define IDD_MODE_CTRL               211
#define IDD_MODE_SHIFT              212
#define IDD_INPUT_KEY               213
#define IDD_INPUT_CTRL              214
#define IDD_INPUT_SHIFT             215
#define IDD_CLAUSE_KEY              216
#define IDD_CLAUSE_CTRL             217
#define IDD_CLAUSE_SHIFT            218
#define IDD_CONVERT_KEY             219
#define IDD_CONVERT_CTRL            220
#define IDD_CONVERT_SHIFT           221
#define IDD_ACCEPT_KEY              222
#define IDD_ACCEPT_CTRL             223
#define IDD_ACCEPT_SHIFT            224
#define IDD_NEXT_KEY                225
#define IDD_NEXT_CTRL               226
#define IDD_NEXT_SHIFT              227
#define IDD_PREV_KEY                228
#define IDD_PREV_CTRL               229
#define IDD_PREV_SHIFT              230
#define IDD_STATIC_OPTIONS          250
#define IDD_STATIC_HOTKEYS          251
#define IDD_STATIC_STARTUP_MODE     252
#define IDD_STATIC_FONT             253
#define IDD_STATIC_INPUT            254
#define IDD_STATIC_MODE             255
#define IDD_STATIC_CLAUSE           256
#define IDD_STATIC_CONVERT          257
#define IDD_STATIC_ACCEPT           258
#define IDD_STATIC_NEXT             259
#define IDD_STATIC_PREV             260

// Note: This is added to the country code to get the actual font dialog ID
#define DLG_FONT_BASE               800
#define DLG_FONT_EN                 801     // English font dialog
#define DLG_FONT_JA                 881     // Japanese font dialog

#define DLG_ABOUT                   900
#define IDD_ICON                    901
#define IDD_VERSION                 902
#define IDD_NOTICES                 903

#define IDM_POPUP                   300
#define IDM_MOVE                    310
#define IDM_FLOAT                   320
#define IDM_SETTINGS                321
#define IDM_ABOUT                   329
#define IDM_CLOSE                   330

#define IDM_INPUT_BASE              400
#define IDM_HIRAGANA                ( IDM_INPUT_BASE + 1 )
#define IDM_KATAKANA                ( IDM_INPUT_BASE + 2 )
#define IDM_FULLWIDTH               ( IDM_INPUT_BASE + 3 )
#define IDM_HALFWIDTH               ( IDM_INPUT_BASE + 4 )

#define ID_HOTKEY_INPUT             1000
#define ID_HOTKEY_MODE              1001
#define ID_HOTKEY_KANJI             1002
#define ID_HOTKEY_CONVERT           1003
#define ID_HOTKEY_ACCEPT            1004
#define ID_HOTKEY_CANCEL            1005
#define ID_HOTKEY_NEXT              1006
#define ID_HOTKEY_PREV              1007
#define ID_CONV_TOP                 1010
#define ID_CONV_UNTOP               1011


// I18N base numbers (these are all 100 * country code + 10000)

#define ID_BASE_EN                  10100       // English
#define ID_BASE_RU                  10700       // Russian
#define ID_BASE_NL                  13100       // Dutch
#define ID_BASE_FR                  13300       // French
#define ID_BASE_ES                  13400       // Spanish
#define ID_BASE_IT                  13900       // Italian
#define ID_BASE_SV                  14600       // Swedish
#define ID_BASE_DE                  14900       // German
#define ID_BASE_JA                  18100       // Japanese
#define ID_BASE_KO                  18200       // Korean
#define ID_BASE_CN                  18600       // Simplified Chinese
#define ID_BASE_TW                  18800       // Traditional Chinese


// String IDs

#define IDS_PRODUCT_NAME            1
#define IDS_PRODUCT_VERSION         2
#define IDS_PRODUCT_COPYRIGHT       3
#define IDS_PRODUCT_NOTICE1         4
#define IDS_PRODUCT_NOTICE2         5
#define IDS_WINDOW_ABOUT            6
#define IDS_COMMON_OK               7
#define IDS_COMMON_CANCEL           8
#define IDS_BTN_INPUT               9
#define IDS_BTN_CLAUSE              10
#define IDS_KEYS_SPACE              11
#define IDS_KEYS_LEFT               12
#define IDS_KEYS_RIGHT              13
#define IDS_KEYS_UP                 14
#define IDS_KEYS_DOWN               15

#define IDS_WINDOW_SETTINGS         20
#define IDS_SET_OPTIONS             21
#define IDS_SET_HOTKEYS             22
#define IDS_OPT_MODE                23
#define IDS_OPT_USELAST             24
#define IDS_OPT_FONT                25
#define IDS_KEY_ACTIVATE            26
#define IDS_KEY_MODE                27
#define IDS_KEY_CLAUSE              28
#define IDS_KEY_CONVERT             29
#define IDS_KEY_ACCEPT              30
#define IDS_KEY_NEXT                31
#define IDS_KEY_PREV                32
#define IDS_KEY_COLLISION           33

#define IDS_FONTDLG_FONT            40
#define IDS_FONTDLG_NAME            41
#define IDS_FONTDLG_STYLE           42
#define IDS_FONTDLG_SAMPLE          43
#define IDS_FONTDLG_SAMPLE_TEXT     44
#define IDS_MODE_JP_HIRAGANA        45
#define IDS_MODE_JP_HIRAGANAKANJI   46
#define IDS_MODE_JP_KATAKANA        47
#define IDS_MODE_JP_KATAKANAKANJI   48
#define IDS_MODE_JP_HANKAKU         49
#define IDS_MODE_JP_HANKAKUKANJI    50
#define IDS_MODE_FULLWIDTH          51
#define IDS_MODE_NONE               52
#define IDS_MODE_KO_HANGUL          53

#define IDS_MENU_FLOAT              60
#define IDS_MENU_SETTINGS           61
#define IDS_MENU_ABOUT              62
#define IDS_MENU_CLOSE              63
#define IDS_MENU_HIRAGANA           64
#define IDS_MENU_KATAKANA           65
#define IDS_MENU_FULLWIDTH          66
#define IDS_MENU_HALFWIDTH          67
#define IDS_MENU_HANGUL             68

#define IDS_ERROR_TITLE             80
#define IDS_ERROR_UCONVOBJECT       81
#define IDS_ERROR_UCONV_CODEPAGE    82
#define IDS_ERROR_ROMKAN            83
#define IDS_ERROR_CONNECTION        84
#define IDS_ERROR_CONVERSION        85
#define IDS_ERROR_JLIB_CONV         86
#define IDS_ERROR_JLIB_STRING       87
#define IDS_ERROR_JLIB_PHRASE       88
#define IDS_ERROR_JLIB_PCONV        89
#define IDS_ERROR_JLIB_PSTRING      90
#define IDS_ERROR_MEMORY            91
#define IDS_ERROR_FONTDLG           92

