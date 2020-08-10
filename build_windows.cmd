libs\buildTool\MSBuild\Current\Bin\MSBuild.exe makeobj\Makeobj.vcxproj -property:Configuration=Release
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-GDI.vcxproj -property:Configuration=Debug
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-SDL2.vcxproj -property:Configuration=Debug

del simutrans\Simutrans_GDI.pdb
del simutrans\Simutrans_GDI.exe.manifest
del simutrans\Simutrans_SDL2.pdb
del simutrans\Simutrans_SDL2.exe.manifest