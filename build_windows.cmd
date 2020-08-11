rem libs\buildTool\MSBuild\Current\Bin\MSBuild.exe makeobj\Makeobj.vcxproj -property:Configuration=Release
rem libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-GDI.vcxproj -property:Configuration=Debug
rem libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-SDL2.vcxproj -property:Configuration=Debug

rem del simutrans\Simutrans_GDI.pdb
rem del simutrans\Simutrans_GDI.exe.manifest
rem del simutrans\Simutrans_SDL2.pdb
rem del simutrans\Simutrans_SDL2.exe.manifest

libs\curl\curl.exe https://translator.simutrans.com/script/main.php?page=wrap
libs\curl\curl.exe https://translator.simutrans.com/data/tab/language_pack-Base+texts.zip > language_pack.zip
libs\7zip\7z.exe x language_pack.zip -osimutrans\text
rem Expand-Archive -Path language_pack.zip -DestinationPath simutrans/text

pause