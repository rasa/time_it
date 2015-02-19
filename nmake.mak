.PHONY:	all clean realclean

all:
	$(MAKE) /f time_it.mak CFG="time_it - Win32 Release"	all
	$(MAKE) /f time_it.mak CFG="time_it - Win32 Debug"	all

clean:
	$(MAKE) /f time_it.mak CFG="time_it - Win32 Release"	clean
	$(MAKE) /f time_it.mak CFG="time_it - Win32 Debug"	clean

realclean: clean
	-cmd /c del /s *.bak *.bsc *.idb *.pdb *.lib *.ncb *.obj *.opt *.pch *.plg *.sbr
