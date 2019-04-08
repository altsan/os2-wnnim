# Makefile for WnnIM test programs
# IBM C/C++ Compiler + NMake(32)

CC         = icc
RC         = rc
LINK       = ilink

# Set as appropriate
WNNINCLUDE = e:\usr\local\include\wnn

# Compiler flags:
# /Gm+      Use multithread library
# /Ge-      Generate DLL
# /I<path>  Add <path> to include path
# /Mc       Use _cdecl linkage by default (only needed for modules that interface to GCC-built APIs)
# /O        Turn on optimizations
# /Rn       Generate subsystem (no runtime)
# /Ss       Allow C++ style comments
# /Wuse     Warn on unused variables

CFLAGS_EXE = /Gm+ /O /Ss /Wuse /I$(WNNINCLUDE) /I..
LFLAGS_EXE = /NOE /NOLOGO /MAP

HEADERS_ALL = ..\codepage.h ..\wnnclient.h ..\wnnhook.h ..\wnnim.h
LIBS = libuls.lib libconv.lib wnn0_dll.lib

!ifdef DEBUG
    CFLAGS_EXE = $(CFLAGS_EXE) /Ti+ /Tm+
    LFLAGS_EXE = $(LFLAGS_EXE) /DEBUG
!endif

.c.obj:
        $(CC) /c $(CFLAGS_EXE) $<

all: testconv.exe
# cwtest.exe

testconv.exe: testconv.obj wnnclient.obj wnnconv.obj codepage.obj
        $(LINK) $(LFLAGS_EXE) /STACK:0x50000 $** $(LIBS) /OUT:$@

cwtest.exe:   cwtest.obj convwin.obj
        $(LINK) $(LFLAGS_EXE) /PMTYPE:PM $** $(LIBS) /OUT:$@

testconv.obj: testconv.c $(HEADERS_ALL)

cwtest.obj:   cwtest.c $(HEADERS_ALL)

wnnclient.obj: ..\wnnclient.c $(HEADERS_ALL)
        $(CC) /c /Mc $(CFLAGS_EXE) $<

wnnconv.obj: ..\wnnconv.c $(HEADERS_ALL)
        $(CC) /c /Mc $(CFLAGS_EXE) $<

codepage.obj: ..\codepage.c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

convwin.obj: ..\convwin.c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

clean:
        rm -f *.exe *.obj *.map

