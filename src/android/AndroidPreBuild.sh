#!/bin/sh

if [ -e simutrans ]; then
  echo Simutrans SVN already downloaded
else
  svn checkout https://github.com/Andarix/simutrans simutrans || exit 1
fi
echo "Updating translations"
  cd simutrans || exit 1
  tools/get_lang_files.sh || exit 1
  rm simutrans/config/simuconf.tab
  mv simutrans/config/simuconf_android.tab simutrans/config/simuconf.tab

cd simutrans

#echo "Get pak64"
#../tools/get_pak.sh pak64 || exit 1

#echo "Get pak64.german"
#../tools/get_pak.sh pak.german || exit 1

#echo "Get pak64.japan"
#../tools/get_pak.sh pak64.japan || exit 1


echo "copy cacert.pem and Android get_pak.sh"
  cp src/android/cacert.pem simutrans/cacert.pem || exit 1
  cp src/android/unpak.sh simutrans/get_pak.sh; chmod 755 simutrans/get_pak.sh  || exit 1
  cd ..

echo "Adding assets"

download_with_retry() {
  echo 'START ' $1
  RESULT=-1
  RETRY=5
  while [ $RESULT -ne 0 ] && [ $RETRY -ne 0 ]
  do
    wget --show-progress $1 -O $2
    RESULT=$?
    RETRY=$((RETRY-1))
  done
  if [ $RETRY -eq 0 ]
    then
      exit 1
  fi
}

#cd simutrans
#[ -e simutrans/pak ] || (download_with_retry http://downloads.sourceforge.net/project/simutrans/pak64/123-0/simupak64-123-0.zip simupak64-123-0.zip && unzip -n simupak64-123-0.zip -d .) || exit 1
#[ -e ../../simutrans/pak.japan ] || (download_with_retry http://downloads.sourceforge.net/project/simutrans/pak64.japan/123-0/simupak64.japan-123-0.zip simupak64.japan-123-0.zip && unzip -n simupak64.japan-123-0.zip -d .) || exit 1
#[ -e ../simutrans/pak64.german ] || (download_with_retry https://simutrans-germany.com/pak.german/pak64.german_0-123-0-0-5_full.zip pak64.german_0-123-0-0-5_full.zip && unzip -n pak64.german_0-123-0-0-5_full.zip -d .) || exit 1

[ -e simutrans/simutrans/music/TimGM6mb.sf2 ] || (download_with_retry https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw simutrans/simutrans/music/TimGM6mb.sf2) || exit 1
#[ -e ../simutrans/font/RobotoCondensed-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto%20Condensed Roboto_Condensed.zip && unzip -n Roboto_Condensed.zip -d ../simutrans/font) || exit 1
#cd ../..
[ -e simutrans/simutrans/font/Roboto-Regular.ttf ] || (download_with_retry https://fonts.google.com/download?family=Roboto Roboto.zip && unzip -n Roboto.zip Roboto-Regular.ttf -d simutrans/simutrans/font) || exit 1

echo 'Done adding assets'
#cd ..

#mkdir -p AndroidData
#echo Generating data.zip
#cd simutrans/simutrans

#rm -f ../../AndroidData/data.zip
#zip -r -0 ../../AndroidData/data.zip * >/dev/null
#cd ../..
#echo Generating data.zip done

#[ -e AndroidData/TimGM6mb.sf2 ] || cp simutrans/TimGM6mb.sf2 AndroidData/ || exit 1
