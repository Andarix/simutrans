
git push --delete "simutrans_build" origin Nightly_a
git tag -d Nightly_a

git tag -a Nightly_a -m ""

git push --tags --progress "simutrans_build" origin

pause
