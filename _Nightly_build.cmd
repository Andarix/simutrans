git svn fetch
git merge --log=20 refs/remotes/origin/trunk

git push --delete "simutrans_build" master Nightly
git tag -d Nightly

git tag -a Nightly -m ""

git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION %string:~1,5% > revision.h

git commit -a

git push --tags --progress "simutrans_build" master

pause