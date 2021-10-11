rmdir /s /q .vs
rmdir /s /q Debug
rmdir /s /q x64\Debug
rmdir /s /q "Generated Files"

del *.sdf
del *.VC.db

del Release\*.obj
del Release\*.log
del Release\*.tlog
del Release\*.lastbuildstate
del Release\*.txt
rmdir /s /q Release\samplerate.tlog

del x64\Release\*.obj
del x64\Release\*.log
del x64\Release\*.txt
rmdir /s /q x64\Release\samplerate.tlog

del "ARM\Release\*.obj"
del "ARM\Release\*.log"
del "ARM\Release\*.resfiles"
del "ARM\Release\*.txt"
del "ARM\Release\*.xml"
del "ARM\Release\*.intermediate"
del "ARM\Release\*.recipe"
rmdir /s /q "ARM\Release\samplerate"
rmdir /s /q "ARM\Release\samplerate.tlog"
rmdir /s /q "ARM\Release\embed"

del Emscripten\Release\*.o
del Emscripten\Release\*.log
del Emscripten\Release\*.tlog
del Emscripten\Release\*.lastbuildstate
del Emscripten\Release\*.cache

rmdir /s /q NX64