git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION %string:~1,5% > revision.h

libs\buildTool\MSBuild\Current\Bin\MSBuild.exe makeobj\Makeobj.vcxproj -property:Configuration=Release
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-GDI.vcxproj -property:Configuration=git_actions
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-SDL2.vcxproj -property:Configuration=git_actions
libs\buildTool\MSBuild\Current\Bin\MSBuild.exe Simutrans-Server.vcxproj -property:Configuration=git_actions

copy Simutrans_GDI.exe simutrans\Simutrans_GDI.exe
copy Simutrans_SDL2.exe simutrans\Simutrans_SDL2.exe
copy Simutrans_Server.exe simutrans\Simutrans_SDL2.exe
copy libs/SDL2.dll simutrans\SDL2.dll


rem libs\curl\curl.exe https://translator.simutrans.com/script/main.php?page=wrap
rem libs\curl\curl.exe https://translator.simutrans.com/data/tab/language_pack-Base+texts.zip > language_pack.zip
rem libs\7zip\7z.exe x language_pack.zip -osimutrans\text
rem Expand-Archive -Path language_pack.zip -DestinationPath simutrans/text

pause