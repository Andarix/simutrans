git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION %string:~1,5% > src/simutrans/revision.h

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" src\makeobj\Makeobj.vcxproj /p:Configuration=Release
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Simutrans-GDI.vcxproj /p:Configuration=git_actions
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Simutrans-SDL2.vcxproj /p:Configuration=git_actions
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Simutrans-Server.vcxproj /p:Configuration=git_actions
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Simutrans-GDI.vcxproj /p:Configuration=git_actions

rem copy Simutrans_GDI.exe simutrans\Simutrans_GDI.exe
rem copy Simutrans_SDL2.exe simutrans\Simutrans_SDL2.exe
rem copy Simutrans_Server.exe simutrans\Simutrans_SDL2.exe
rem copy libs/SDL2.dll simutrans\SDL2.dll


rem libs\curl\curl.exe https://translator.simutrans.com/script/main.php?page=wrap
rem libs\curl\curl.exe https://translator.simutrans.com/data/tab/language_pack-Base+texts.zip > language_pack.zip
rem libs\7zip\7z.exe x language_pack.zip -osimutrans\text
rem Expand-Archive -Path language_pack.zip -DestinationPath simutrans/text

pause