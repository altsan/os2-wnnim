# Makefile for WnnIM + WnnHook
# IBM C/C++ Compiler + NMake(32)

CC         = icc
RC         = rc
LINK       = ilink

# Compiler flags:
# /Gm+      Use multithread library
# /Ge-      Generate DLL
# /I<path>  Add <path> to include path
# /O        Turn on optimizations
# /Rn       Generate subsystem (no runtime)
# /Ss       Allow C++ style comments
# /Wuse     Warn on unused variables

CFLAGS_EXE = /Gm+ /O /Ss /Wuse /Ie:\usr\local\include\wnn
CFLAGS_DLL = /Ge- /Rn /O /Ss /Wuse

LFLAGS_EXE = /NOE /PMTYPE:PM /NOLOGO /MAP /STACK:0x50000
LFLAGS_DLL = /NOE /DLL /NOLOGO /MAP

HEADERS_ALL = ids.h wnnhook.h codepage.h

EXE = wnnim
OBJS_EXE = $(EXE).obj codepage.obj wnnclient.obj
LIBS_EXE = libuls.lib libconv.lib wnn0_dll.lib

DLL = wnnhook
OBJS_DLL = $(DLL).obj

!ifdef DEBUG
    CFLAGS_EXE = $(CFLAGS_EXE) /Ti+ /Tm+
    CFLAGS_DLL = $(CFLAGS_DLL) /Ti+ /Tm+
    LFLAGS_EXE = $(LFLAGS_EXE) /DEBUG
    LFLAGS_DLL = $(LFLAGS_DLL) /DEBUG
!endif

.c.obj: $(HEADERS)
        $(CC) /c $(CFLAGS_EXE) $<

.all: $(DLL).dll $(EXE).exe

$(EXE).exe: $(OBJS_EXE) $(EXE).res $(DLL).lib
        $(LINK) $(LFLAGS_EXE) $(OBJS_EXE) $(DLL).lib $(LIBS_EXE) /OUT:$@
        rc -x $(EXE).res $(EXE).exe

$(EXE).obj: $(EXE).c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

wnnclient.obj: wnnclient.c $(HEADERS_ALL)
        $(CC) /c /Mc $(CFLAGS_EXE) $<

codepage.obj: codepage.c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

$(EXE).res: $(EXE).rc $(EXE).dlg ids.h app.ico
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

