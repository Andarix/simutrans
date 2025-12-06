rem git svn fetch
wsl git svn fetch
rem cd /d/simutrans_entwicklung/svn/simutrans_git/
rem git svn fetch

rem exit

git merge --log=20 refs/remotes/origin/trunk

rem wsl git svn log --oneline --limit=1 >status.txt

echo Revision in status.txt eintragen
pause
set /p string=<status.txt

echo #define REVISION %string:~1,5% > src/simutrans/revision.h
echo #define REVISION %string:~1,5% > revision.h

git add src/simutrans/revision.h
git add revision.h

pause

git commit -am "r%string:~1,5%"

pause

git push --delete "simutrans_build" origin Nightly
git tag -d Nightly

git tag -a Nightly -m ""

git push --tags --progress "simutrans_build" origin

pause
