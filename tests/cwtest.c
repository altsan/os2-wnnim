#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uconv.h>

#define ErrorPopup( text ) \
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, text, "Error", 0, MB_OK | MB_ERROR )

#include "convwin.h"
#include "wnnhook.h"


/* ------------------------------------------------------------------------- *
 * ------------------------------------------------------------------------- */
int main( void )
{
    HAB   hab;                      // anchor block handle
    HMQ   hmq;                      // message queue handle
    HWND  hwndFrame,                // handles to windows
          hwndClient;
    QMSG  qmsg;                     // message queue
    LONG  scr_cx, scr_cy,           // screen size coordinates
          win_cx, win_cy;           // initial window size
    ULONG flFrameOpts = FCF_STANDARD & ~FCF_SHELLPOSITION & ~FCF_ICON & ~FCF_MENU & ~FCF_ACCELTABLE;
    WNDPARAMS wp  = {0};
    CWCTLDATA cwd = {0};
    UniChar   aus[ 11 ];
    USHORT    ausPhrase[ 3 ];

    aus[0] = L'T';
    aus[1] = L'e';
    aus[2] = L's';
    aus[3] = L't';
    aus[4] = L' ';
    aus[5] = 0x65e5;
    aus[6] = 0x672c;
    aus[7] = 0x8a9e;
    aus[8] = 0x5165;
    aus[9] = 0x529b;
    aus[10] = 0;

    ausPhrase[ 0 ] = 4;
    ausPhrase[ 1 ] = 7;
    ausPhrase[ 2 ] = 9;

    hab = WinInitialize( 0 );
    hmq = WinCreateMsgQueue( hab, 0 );
    CWinRegisterClass( hab );

    hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0, &flFrameOpts,
                                    WC_WNNIMCONVWIN, "Conversion Window Test",
                                    0L, 0, 0, &hwndClient );
    cwd.cb = sizeof( CWCTLDATA );
    cwd.fsMode = MODE_JP;
    wp.fsStatus = WPM_CTLDATA;
    wp.cbCtlData = cwd.cb;
    wp.pCtlData = &cwd;
    WinSendMsg( hwndClient, WM_SETWINDOWPARAMS, MPFROMP( &wp ), 0L );

    win_cx = 300;
    win_cy = 100;
    scr_cx = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
    scr_cy = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
    WinSetWindowPos( hwndFrame, 0, (scr_cx/2)-(win_cx/2), (scr_cy/2)-(win_cy/2),
                     win_cx, win_cy, SWP_MOVE | SWP_SIZE );

    WinSetPresParam( hwndClient, PP_FONTNAMESIZE, 25, "10.Times New Roman MT 30");
    WinSendMsg( hwndClient, CWM_SETTEXT, MPFROM2SHORT( CWT_ALL, 10 ), MPFROMP( aus ));
    WinSendMsg( hwndClient, CWM_SETPHRASES, MPFROMSHORT( 3 ), MPFROMP( ausPhrase ));
//    WinSendMsg( hwndClient, CWM_SETTEXT, MPFROM2SHORT( 0, 6 ), MPFROMP( L"Toast "));
    WinSendMsg( hwndClient, CWM_SELECTPHRASE, MPFROMSHORT( CWT_FIRST ), 0L );
    WinSendMsg( hwndClient, CWM_SELECTPHRASE, MPFROMSHORT( CWT_NEXT ), 0L );

    WinSetWindowPos( hwndFrame, 0, 0, 0, 0, 0, SWP_ACTIVATE | SWP_SHOW );

    while ( WinGetMsg( hab, &qmsg, 0, 0, 0 )) WinDispatchMsg( hab, &qmsg );

    WinDestroyWindow( hwndFrame );
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

    return ( 0 );
}



