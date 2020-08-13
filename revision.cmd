git svn log --oneline --limit=1 > status.txt

set /p string=<status.txt

echo #define REVISION = %string:~1,5% > revision.h
