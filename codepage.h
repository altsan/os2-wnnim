/****************************************************************************
 * codepage.h                                                               *
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

// Codepage numbers
#define CP_UNICODE          1200        // UCS-2
#define CP_EUC_JP           954         // EUC-JP
#define CP_EUC_KR           970         // EUC-KR
#define CP_EUC_CN           1383        // EUC-CN
#define CP_EUC_TW           964         // EUC-TW
#define CP_SJIS             932         // Japanese Shift-JIS
#define CP_KSCODE           949         // Korean KS-Code
#define CP_BIG5             950         // Taiwanese BIG-5
#define CP_GBK              1386        // Chinese GBK


BOOL   _System IsDBCSLeadByte( CHAR ch, PBYTE pDBCS );
USHORT _System GetEucCodepage( USHORT usLang );
ULONG  _System CreateUconvObject( ULONG ulCP, UconvObject *uconv );
ULONG  _System StrConvert( PCH pchInput, PCH pchOutput, UconvObject uconvFrom, UconvObject uconvTo );
USHORT _System ConvertFullWidth( PSZ pszInput, UniChar *puczOutput, USHORT usMax );

