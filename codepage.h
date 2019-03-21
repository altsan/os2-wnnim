BOOL   _Optlink IsDBCSLeadByte( CHAR ch, PBYTE pDBCS );
USHORT _Optlink GetEucCodepage( USHORT usLang );
ULONG  _Optlink CreateUconvObject( ULONG ulCP, UconvObject *uconv );
PSZ    _Optlink StrConvert( PSZ pszInput, PSZ pszOutput, UconvObject uconvFrom, UconvObject uconvTo );



