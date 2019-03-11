# Makefile for WnnIM + WnnHook
# IBM C/C++ Compiler + NMake(32)

CC         = icc
RC         = rc
LINK       = ilink

# Compiler flags:
# /Gm+      Use multithread library
# /Ge-      Generate DLL
# /O        Turn on optimizations
# /Rn       Generate subsystem (no runtime)
# /Ss       Allow C++ style comments
# /Wuse     Warn on unused variables

CFLAGS_EXE = /Gm+ /O /Ss /Wuse
CFLAGS_DLL = /Ge- /Rn /O /Ss /Wuse

LFLAGS_EXE = /NOE /PMTYPE:PM /NOLOGO /MAP
LFLAGS_DLL = /NOE /DLL /NOLOGO /MAP

HEADERS_ALL = ids.h wnnhook.h codepage.h

EXE = wnnim
OBJS_EXE = $(EXE).obj codepage.obj

DLL = wnnhook
OBJS_DLL = $(DLL).obj

!ifdef DEBUG
    CFLAGS_EXE = $(CFLAGS_EXE) /Ti+ /Tm+
    LFLAGS_EXE = $(LFLAGS_EXE) /DEBUG
!endif


.all: $(DLL).dll $(EXE).exe

$(EXE).exe: $(OBJS_EXE) $(EXE).res $(DLL).lib
        $(LINK) $(LFLAGS_EXE) $(OBJS_EXE) $(DLL).lib /OUT:$@
        rc -x $(EXE).res $(EXE).exe

$(EXE).obj: $(EXE).c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

fonts.obj: fonts.c $(HEADERS_ALL)
        $(CC) /c $(CFLAGS_EXE) $<

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

