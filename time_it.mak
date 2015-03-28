# Microsoft Developer Studio Generated NMAKE File, Based on time_it.dsp
!IF "$(CFG)" == ""
CFG=time_it - Win32 Debug
!MESSAGE No configuration specified. Defaulting to time_it - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "time_it - Win32 Release" && "$(CFG)" != "time_it - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "time_it.mak" CFG="time_it - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "time_it - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "time_it - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "time_it - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\time_it.exe"


CLEAN :
	-@erase "$(INTDIR)\time_it.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\time_it.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\shared" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\time_it.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\time_it.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=shared.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\time_it.pdb" /machine:I386 /out:"$(OUTDIR)\time_it.exe" 
LINK32_OBJS= \
	"$(INTDIR)\time_it.obj"

"$(OUTDIR)\time_it.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "time_it - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\time_it.exe" "$(OUTDIR)\time_it.bsc"


CLEAN :
	-@erase "$(INTDIR)\time_it.obj"
	-@erase "$(INTDIR)\time_it.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\time_it.bsc"
	-@erase "$(OUTDIR)\time_it.exe"
	-@erase "$(OUTDIR)\time_it.ilk"
	-@erase "$(OUTDIR)\time_it.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\shared" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\time_it.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\time_it.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\time_it.sbr"

"$(OUTDIR)\time_it.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=sharedd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\time_it.pdb" /debug /machine:I386 /out:"$(OUTDIR)\time_it.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\time_it.obj"

"$(OUTDIR)\time_it.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("time_it.dep")
!INCLUDE "time_it.dep"
!ELSE 
!MESSAGE Warning: cannot find "time_it.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "time_it - Win32 Release" || "$(CFG)" == "time_it - Win32 Debug"
SOURCE=.\time_it.cpp

!IF  "$(CFG)" == "time_it - Win32 Release"


"$(INTDIR)\time_it.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "time_it - Win32 Debug"


"$(INTDIR)\time_it.obj"	"$(INTDIR)\time_it.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

