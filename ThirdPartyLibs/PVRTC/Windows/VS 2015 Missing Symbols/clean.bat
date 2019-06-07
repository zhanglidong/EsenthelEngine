rmdir /s /q .vs
del *.sdf

del Release\*.obj
del Release\*.log
del Release\*.tlog
del Release\*.txt
del Release\*.lastbuildstate
del Release\*.unsuccessfulbuild
rmdir /s /q "Release\VS 2015 .791AA268.tlog"

del x64\Release\*.obj
del x64\Release\*.log
del x64\Release\*.tlog
del x64\Release\*.txt
del x64\Release\*.lastbuildstate
del x64\Release\*.unsuccessfulbuild
rmdir /s /q "x64\Release\VS 2015 .791AA268.tlog"