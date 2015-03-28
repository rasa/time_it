all:
	MSBuild.exe /nologo time_it.sln /p:Configuration=Debug
	MSBuild.exe /nologo time_it.sln /p:Configuration=Release

clean:
	MSBuild.exe /nologo time_it.sln /p:Configuration=Debug   /t:clean
	MSBuild.exe /nologo time_it.sln /p:Configuration=Release /t:clean

upgrade:
	devenv time_it.sln /upgrade
	
.PHONY:	all clean upgrade


