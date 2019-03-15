/****************************************************************************
 * wnnhook.c                                                                *
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
#define INCL_DOSERRORS
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_WIN
#include <os2.h>
#include <stdio.h>

#include "wnnhook.h"
#include "ids.h"

BOOL      g_fHookActive;
HMODULE   g_hMod;
HAB       g_hab;
HMQ       g_hmq;
HWND      g_hwndClient;
HATOMTBL  g_hATSys;
WNNSHARED global;


/* -------------------------------------------------------------------------- *
 * Clear a character buffer (cheap and dirty memset replacement).             *
 * -------------------------------------------------------------------------- *
void ClearBuffer( CHAR *pBuf, USHORT usLen )
{
    USHORT i;
    for ( i = 0; pBuf && i < usLen; i++ ) pBuf[ i ] = 0;
}
*/


/* -------------------------------------------------------------------------- *
 * Hook for posted messages.                                                  *
 * Return TRUE for messages processed here, or FALSE to pass the message on.  *
 * -------------------------------------------------------------------------- */
BOOL EXPENTRY WnnHookInput( HAB hab, PQMSG pQmsg, USHORT fs )
{
    CHAR   c;
    ULONG  ulKey;
    USHORT fsFlags;

    switch( pQmsg->msg ) {
        case WM_CHAR:
            fsFlags = SHORT1FROMMP( pQmsg->mp1  );
            if ( fsFlags & KC_KEYUP ) break;    // don't process key-up events

            ulKey = (ULONG) pQmsg->mp2;
            if (( fsFlags & VK_CTRL ) && ( SHORT2FROMMP( pQmsg->mp2 ) == VK_SPACE )) {   // global.fsKeyMode, global.usKeyMode
                // input mode hotkey
                WinPostMsg( g_hwndClient, WM_COMMAND,
                            MPFROMSHORT( ID_HOTKEY_MODE ),
                            MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
                return TRUE;
            }
                    /*
                // Rationalize some of the virtual key-codes for comparison
                if ( ulKey & VK_ENTER ) {
                    ulKey |= VK_NEWLINE;
                    ulKey &= ~VK_ENTER;
                }




                    else if ( ulKey == global.ulKeyCJK ) {      // CJK mode hotkey
                        WinPostMsg( g_hwndClient, WM_COMMAND, MPFROMSHORT( IDD_KANJI ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
                        return TRUE;
                    }
                    else if ( global.fsMode & MODE_CJK ) {
                        if ( ulKey == global.ulKeyConvert ) {       // CJK convert hotkey
                            WinPostMsg( g_hwndClient, WM_COMMAND, MPFROMSHORT( IDD_CONVERT ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
                            return TRUE;
                        }
                        else if ( ulKey == global.ulKeyAccept ) {   // CJK accept hotkey
                            WinPostMsg( g_hwndClient, WM_COMMAND, MPFROMSHORT( IDD_ACCEPT ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));
                            return TRUE;
                        }
                    }
                    */

            if ( fsFlags & KC_CHAR ) {
                c = (CHAR)( SHORT1FROMMP( pQmsg->mp2 ) & 0xFF );

                if ( global.fsMode & 0xFF ) {           // any conversion mode is active
                    if ( c > 0x20 && c < 0x7E ) {               // convertible byte value
                        global.hwndSource = pQmsg->hwnd;
                        WinPostMsg( g_hwndClient, global.wmAddChar, pQmsg->mp1, pQmsg->mp2 );
                        return TRUE;
                    }
                }
            }
            break;

    }
    return FALSE;
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 * -------------------------------------------------------------------------- */
PWNNSHARED EXPENTRY _Export WnnGlobalData( void )
{
    return &global;
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 * -------------------------------------------------------------------------- */
BOOL EXPENTRY _Export WnnHookInit( HWND hwnd )
{
#ifndef TESTHOOK
    g_hmq = NULLHANDLE;
#else
    g_hmq = HMQ_CURRENT;
#endif

    g_hwndClient = hwnd;
    if ( g_fHookActive ) return TRUE;

    g_hATSys = WinQuerySystemAtomTable();
    global.wmAddChar = WinAddAtom( g_hATSys, "WnnAddChar");
/*
    global.wmCJKMode = WinAddAtom( g_hATSys, "WnnCJKMode");
    global.wmInputMode = WinAddAtom( g_hATSys, "WnnInputMode");
    global.wmConvertCJK = WinAddAtom( g_hATSys, "WnnConvertCJK");
    global.wmAccept = WinAddAtom( g_hATSys, "WnnAccept");
*/

    // Default hotkeys (should eventually be configurable)
    global.ulKeyMode    = VK_SPACE;
    global.ulKeyCJK     = VK_ALT | '`';
    global.ulKeyConvert = VK_SPACE;
    global.ulKeyAccept  = VK_NEWLINE;

    if ( DosQueryModuleHandle("wnnhook", &g_hMod )) return FALSE;
    g_hab = WinQueryAnchorBlock( hwnd );
    WinSetHook( g_hab, g_hmq, HK_INPUT, (PFN) WnnHookInput, g_hMod );
    g_fHookActive = TRUE;

    return TRUE;
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 * -------------------------------------------------------------------------- */
BOOL EXPENTRY _Export WnnHookTerm( void )
{
    if ( g_fHookActive ) {
        WinReleaseHook( g_hab, g_hmq, HK_INPUT, (PFN) WnnHookInput, g_hMod );
        WinDeleteAtom( g_hATSys, global.wmAddChar );
/*
        WinDeleteAtom( g_hATSys, global.wmCJKMode );
        WinDeleteAtom( g_hATSys, global.wmInputMode );
        WinDeleteAtom( g_hATSys, global.wmConvertCJK );
        WinDeleteAtom( g_hATSys, global.wmAccept );
*/
        g_fHookActive = FALSE;
    }

    return TRUE;
}

