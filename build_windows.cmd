libs\buildTool\MSBuild\Current\Bin\MSBuild.exe makeobj\Makeobj.vcxproj -property:Configuration=Release
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-GDI.vcxproj -property:Configuration=Debug
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-SDL2.vcxproj -property:Configuration=Debug

del simutrans\Simutrans_GDI.pdb
del simutrans\Simutrans_GDI.exe.manifest
del simutrans\Simutrans_SDL2.pdb
del simutrans\Simutrans_SDL2.exe.manifest

libs\curl\curl.exe -L -d "version=0&choice=all&submit=Export%21" https://translator.simutrans.com/script/main.php?page=wrap >NULL
libs\curl\curl.exe -L https://translator.simutrans.com/data/tab/language_pack-Base+texts.zip > language_pack-Base+texts.zip
Expand-Archive -LiteralPath "language_pack-Base+texts.zip" -DestinationPath simutrans/text

pause