rmdir /s /q .vs
rmdir /s /q Debug
rmdir /s /q ipch
rmdir /s /q Emscripten

rmdir /s /q "Engine\Android\obj"
rmdir /s /q "Engine\Arm"
rmdir /s /q "Engine\Asm"
rmdir /s /q "Engine\Debug DX11"
rmdir /s /q "Engine\Debug GL"
rmdir /s /q "Engine\Debug Universal DX11"
rmdir /s /q "Engine\Emscripten"
rmdir /s /q "Engine\Generated Files"
rmdir /s /q "Engine\Release DX11"
rmdir /s /q "Engine\Release GL"
rmdir /s /q "Engine\Release Universal DX11"
rmdir /s /q "Engine\x64"
rmdir /s /q "Engine\NX64"

rmdir /s /q "Project\Arm"
rmdir /s /q "Project\Debug DX11"
rmdir /s /q "Project\Debug GL"
rmdir /s /q "Project\Debug Universal DX11"
rmdir /s /q "Project\Emscripten"
rmdir /s /q "Project\Generated Files"
rmdir /s /q "Project\Release DX11"
rmdir /s /q "Project\Release GL"
rmdir /s /q "Project\Release Universal DX11"
rmdir /s /q "Project\x64"
rmdir /s /q "Project\NX64"

rmdir /s /q "x64"

del "Engine\*.lib"
del "Engine\*.sdf"
del "Project\*.pdb"

rmdir /s /q "Editor\Projects\_Build_"
rmdir /s /q "Editor\Projects\_Publish_"

rmdir /s /q "Editor Source\_Build_\Calculator"
rmdir /s /q "Editor Source\_Build_\Esenthel\Bin"
rmdir /s /q "Editor Source\_Build_\Esenthel\Release DX9"
rmdir /s /q "Editor Source\_Build_\Esenthel\x64"
rmdir /s /q "Editor Source\_Build_\Esenthel\Esenthel.pdb"
rmdir /s /q "Editor Source\_Build_\File Browser"
rmdir /s /q "Editor Source\_Build_\Functions"
rmdir /s /q "Editor Source\_Build_\Server"
rmdir /s /q "Editor Source\_Build_\Uploader"
rmdir /s /q "Editor Source\_Publish_"

rmdir /s /q "_Projects\_Build_"
rmdir /s /q "_Projects\_Publish_"

rmdir /s /q "ThirdPartyLibs\Bullet\lib\build3\vs2010\.vs"
rmdir /s /q "ThirdPartyLibs\De265\bin\.vs"
rmdir /s /q "ThirdPartyLibs\Heif\bin\.vs"
rmdir /s /q "ThirdPartyLibs\JpegTurbo\Windows32\.vs"
rmdir /s /q "ThirdPartyLibs\JpegTurbo\Windows64\.vs"
rmdir /s /q "ThirdPartyLibs\Opus\Windows\.vs"
rmdir /s /q "ThirdPartyLibs\QCMS\Windows\.vs"
rmdir /s /q "ThirdPartyLibs\x265\lib\build\vc15-x86_64\.vs"
rmdir /s /q "ThirdPartyLibs\Xml2\lib\win32\VC10\.vs"

rmdir /s /q "..\Esenthel Website\download\Patcher\Esenthel"