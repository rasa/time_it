.PHONY:	all clean realclean

all:
	MSBuild.exe time_it.sln /t:Rebuild

clean:
	MSBuild.exe time_it.sln /t:Clean

realclean: clean
	-cmd /c del /s *.bak *.bsc *.idb *.pdb *.lib *.ncb *.obj *.opt *.pch *.plg *.sbr
