git svn fetch
git merge --log=20 refs/remotes/origin/trunk

git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION %string:~1,5% > revision.h

git add revision.h

pause
