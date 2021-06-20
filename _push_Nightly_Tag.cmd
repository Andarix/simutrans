
git push --delete "simutrans_build" master Nightly
git tag -d Nightly

git tag -a Nightly -m ""

git push --tags --progress "simutrans_build" master

pause