
git push --delete "simutrans_build" master Nightly_a
git tag -d Nightly_a

git tag -a Nightly_a -m ""

git push --tags --progress "simutrans_build" master

pause
