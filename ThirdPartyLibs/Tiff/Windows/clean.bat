rmdir /s /q .vs

del *.sdf
del *.VC.db

rmdir /s /q Debug

del Release\*.obj
del Release\*.log
rmdir /s /q Release\tiff.tlog

del "Release Universal\*.obj"
del "Release Universal\*.log"
del "Release Universal\*.recipe"
del "Release Universal\*.txt"
rmdir /s /q "Release Universal\tiff.tlog"

del x64\Release\*.obj
del x64\Release\*.log
rmdir /s /q x64\Release\tiff.tlog

del "x64\Release Universal\*.obj"
del "x64\Release Universal\*.log"
del "x64\Release Universal\*.recipe"
del "x64\Release Universal\*.txt"
rmdir /s /q "x64\Release Universal\tiff.tlog"

del ARM\Release\*.obj
del ARM\Release\*.log
rmdir /s /q ARM\Release\tiff.tlog

del Emscripten\Release\*.o
del Emscripten\Release\*.log
del Emscripten\Release\*.tlog
del Emscripten\Release\*.lastbuildstate