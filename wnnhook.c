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
 * -------------------------------------------------------------------------- */
void ClearBuffer( CHAR *pBuf, USHORT usLen )
{
    USHORT i;
    for ( i = 0; pBuf && i < usLen; i++ ) pBuf[ i ] = 0;
}


/* -------------------------------------------------------------------------- *
 * Hook for posted messages.                                                  *
 * Return TRUE for messages processed here, or FALSE to pass the message on.  *
 * -------------------------------------------------------------------------- */
BOOL EXPENTRY WnnHookInput( HAB hab, PQMSG pQmsg, USHORT fs )
{
    CHAR   c;
    USHORT fsFlags;

    switch( pQmsg->msg ) {
        case WM_CHAR:
            fsFlags = SHORT1FROMMP( pQmsg->mp1  );
            if (( fsFlags & KC_CHAR ) && !( fsFlags & KC_KEYUP )) {
                c = (CHAR)( SHORT1FROMMP( pQmsg->mp2 ) & 0xFF );
                if (( global.fsMode & 0xFF ) && ( c >= 0x20 && c <= 0x7E )) {
                    WinPostMsg( g_hwndClient, global.wmAddChar,
                                MPFROMSHORT( SHORT1FROMMP( pQmsg->mp2 )),
                                (MPARAM) pQmsg->hwnd );
                    return TRUE;
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

#ifdef USE_EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;
    LoadExceptq(&exRegRec, "I", "WNNHOOK");
#endif

    ClearBuffer( global.szRomaji, MAX_CHAR_BUF );
    ClearBuffer( global.szKana, MAX_KANA_BUF );

    g_hwndClient = hwnd;
    if ( g_fHookActive ) return TRUE;

    g_hATSys = WinQuerySystemAtomTable();
    // We probably don't need this since we're only sending the message to the client
    //   srand( (UINT)hwnd );
    //   sprintf( global.szAddChar, "WnnAddChar%d", rand() );
    sprintf( global.szAddChar, "WnnAddChar");
    global.wmAddChar = WinAddAtom( g_hATSys, global.szAddChar );
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
        g_fHookActive = FALSE;
    }

#ifdef USE_EXCEPTQ
    UninstallExceptq(&exRegRec);
#endif

    return TRUE;
}

