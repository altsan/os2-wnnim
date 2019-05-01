# Makefile for WnnIM + WnnHook
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

CFLAGS_EXE = /Gm+ /O /Ss /Wuse /I$(WNNINCLUDE)
CFLAGS_DLL = /Ge- /Rn /O /Ss /Wuse

LFLAGS_EXE = /NOE /PMTYPE:PM /NOLOGO /MAP /STACK:0x50000
LFLAGS_DLL = /NOE /DLL /NOLOGO /MAP

HEADERS_ALL = codepage.h ids.h settings.h clipfuncs.h wnnclient.h wnnhook.h wnnim.h

EXE      = wnnim
OBJS_EXE = $(EXE).obj codepage.obj settings.obj wnnclient.obj wnnconv.obj convwin.obj clipfuncs.obj
LIBS_EXE = libuls.lib libconv.lib wnn0_dll.lib

DLL = wnnhook
OBJS_DLL = $(DLL).obj

PMPF=1

!ifdef PMPF
    LIBS_EXE = $(LIBS_EXE) pmprintf.lib
    CFLAGS_EXE = $(CFLAGS_EXE) /D_PMPRINTF_
!endif

!ifdef DEBUG
    CFLAGS_EXE = $(CFLAGS_EXE) /Ti+ /Tm+
    CFLAGS_DLL = $(CFLAGS_DLL) /Ti+ /Tm+
    LFLAGS_EXE = $(LFLAGS_EXE) /DEBUG
    LFLAGS_DLL = $(LFLAGS_DLL) /DEBUG
!endif

.c.obj:
        $(CC) /c $(CFLAGS_EXE) $<

.all: $(DLL).dll $(EXE).exe

$(EXE).exe: $(OBJS_EXE) $(EXE).res $(DLL).lib $(HEADERS_ALL)
        $(LINK) $(LFLAGS_EXE) $(OBJS_EXE) $(DLL).lib $(LIBS_EXE) /OUT:$@
        rc -x $(EXE).res $(EXE).exe
        mapxqs $(EXE).map

$(EXE).obj: $(EXE).c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

wnnclient.obj: wnnclient.c $(HEADERS_ALL)
        $(CC) /c /Mc $(CFLAGS_EXE) $<

wnnconv.obj: wnnconv.c $(HEADERS_ALL)
        $(CC) /c /Mc $(CFLAGS_EXE) $<

convwin.obj: convwin.c $(HEADERS_ALL)

codepage.obj: codepage.c $(HEADERS_ALL)

clipfuncs.obj: clipfuncs.c $(HEADERS_ALL)

settings.obj: settings.c $(HEADERS_ALL)

$(EXE).res: $(EXE).rc $(EXE).dlg ids.h ime.ico
        $(RC) -r $< $@

$(DLL).lib: $(DLL).dll $(DLL).def
        implib $@ $<

$(DLL).dll:  $(OBJS_DLL)
        $(LINK) $(LFLAGS_DLL) $(DLL).obj $(DLL).def /OUT:$@

$(DLL).obj: $(DLL).c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_DLL) $<

clean:
        rm -f $(EXE).exe $(OBJS_EXE) $(EXE).res $(EXE).map
        rm -f $(DLL).dll $(OBJS_DLL) $(DLL).lib $(DLL).map

