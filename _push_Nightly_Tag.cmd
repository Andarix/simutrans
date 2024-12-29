
git push --delete "simutrans_build" origin Nightly
git tag -d Nightly

git tag -a Nightly -m ""

git push --tags --progress "simutrans_build" origin

pause
