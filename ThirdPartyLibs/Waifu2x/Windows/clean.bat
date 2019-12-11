rmdir /s /q .vs
rmdir /s /q Debug
rmdir /s /q "Generated Files"

del *.sdf
del *.VC.db

del Release\*.obj
del Release\*.log
del Release\*.tlog
del Release\*.txt
del Release\*.lastbuildstate
rmdir /s /q Release\Waifu2x.tlog

del x64\Release\*.obj
del x64\Release\*.log
del x64\Release\*.txt
rmdir /s /q x64\Release\Waifu2x.tlog

del "ARM\Release\*.obj"
del "ARM\Release\*.log"
del "ARM\Release\*.resfiles"
del "ARM\Release\*.txt"
del "ARM\Release\*.xml"
del "ARM\Release\*.intermediate"
rmdir /s /q "ARM\Release\Waifu2x"
rmdir /s /q "ARM\Release\Waifu2x.tlog"

del Emscripten\Release\*.o
del Emscripten\Release\*.log
del Emscripten\Release\*.tlog
del Emscripten\Release\*.lastbuildstate