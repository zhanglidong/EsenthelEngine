### Recompiling Shaders
Shader sources are located inside [Engine/Source/Shaders](../Engine/Source/Shaders) folder.

Sources are compiled into binary format and stored inside [Data/Shader](../Data/Shader) folder, which has sub-folders for different platforms and shader models (such as "4" - DirectX 11 Shader Model 4+, and "GL" - OpenGL)

These shader binary files are further placed inside ["Engine.pak"](../Editor/Bin) file (all resources needed by the engine to start) by the ["Esenthel Builder"](../) tool (using the "Create Engine.pak" button)


Currently compiling shaders is supported only from the Windows platform.

In order to recompile the shaders, please:
* open Esenthel Solution file [Esenthel.sln](../Esenthel.sln)
* modify the [Engine/Source/Graphics/Shader Compilers.cpp](../Engine/Source/Graphics/Shader%20Compilers.cpp) file
* enable the "#define COMPILE_DX 1" (to compile for DirectX)
* and uncomment which shader files should be recompiled (such as MAIN, DEFERRED, etc.)
* "MAIN" includes all the basic shaders for drawing on the screen (lines, rectangles, text, ..) and other similar which are always used
* after making that change, running any project will compile selected shaders after 'InitPre' and before 'Init' stage
* shaders will be stored in the 'DataPath' folder
* so if using the "Project" project from Esenthel Solution file, 'DataPath' should be set inside 'InitPre' as: DataPath("../Data"); which will store shaders inside [Data/Shader](../Data/Shader)
* once the shaders are recompiled, don't forget to use "Esenthel Builder" and create the new "Engine.pak" so it can be used by the Editor