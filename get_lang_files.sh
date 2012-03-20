#!/bin/bash
#
# script to fetch language files
#
cd simutrans/text
# get the translations for basis
# The first file is longer, but only because it contains SQL error messages
# - discard it after complete download (although parsing it would give us the archive's name):
wget --post-data "version=0&choice=all&submit=Export!"  --delete-after http://simutrans-germany.com/translator/script/main.php?page=wrap || {
  echo "Error: generating file language_pack-Base+texts.zip failed (wget returned $?)" >&2;
  exit 3;
}
wget -N http://simutrans-germany.com/translator/data/tab/language_pack-Base+texts.zip || {
  echo "Error: download of file language_pack-Base+texts.zip failed (wget returned $?)" >&2
  rm -f "language_pack-Base+texts.zip"
  exit 4
}
unzip -tv "language_pack-Base+texts.zip" || {
   echo "Error: file language_pack-Base+texts.zip seems to be defective" >&2
   rm -f "language_pack-Base+texts.zip"
   exit 5
}
unzip "language_pack-Base+texts.zip"
rm language_pack-Base+texts.zip
# remove Chris English (may become british ... )
rm ce.tab
# Remove check test
rm xx.tab
rm -rf xx
cd ../..
