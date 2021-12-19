git svn fetch
git merge --log=20 refs/remotes/origin/trunk

git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION %string:~1,5% > revision.h

git add revision.h

pause

git commit -am "r%string:~1,5%"

pause

git push --delete "simutrans_build" master Nightly
git tag -d Nightly

git tag -a Nightly -m ""

git push --tags --progress "simutrans_build" master

pause