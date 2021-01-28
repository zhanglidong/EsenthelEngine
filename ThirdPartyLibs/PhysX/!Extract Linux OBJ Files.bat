cd physx\bin\linux.clang\release
rmdir /s /q obj
mkdir obj
cd obj
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysX_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXCharacterKinematic_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXCommon_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXCooking_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXExtensions_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXFoundation_static_64.a
..\..\..\..\..\..\..\Engine\Android\cygwin\ar.exe -x ..\libPhysXVehicle_static_64.a
