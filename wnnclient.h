/****************************************************************************
 * wnnclient.h                                                              *
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

#define WNN_SERVER_ENV_JA   "JSERVER"
#define WNN_SERVER_ENV_KR   "KSERVER"
#define WNN_SERVER_ENV_CN   "CSERVER"
#define WNN_SERVER_ENV_TW   "TSERVER"


INT  _Optlink InitConversionMethod( USHORT usLang, PVOID *ppSession );
void _Optlink FinishConversionMethod( PVOID pSession );
INT  _Optlink InitInputMethod( PSZ pszPath, USHORT usLang );
void _Optlink FinishInputMethod( void );
BYTE _Optlink ConvertPhonetic( USHORT fsMode );


