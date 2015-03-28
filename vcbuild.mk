all:
	VCBuild.exe /nologo time_it.vcproj /rebuild

clean:
	VCBuild.exe /nologo time_it.vcproj /clean
	
upgrade:
	devenv time_it.sln /upgrade

.PHONY:	all clean upgrade

