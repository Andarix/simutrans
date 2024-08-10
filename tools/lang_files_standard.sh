#!/bin/bash
#
# script to fetch language files
#
OUTPUT_DIR=simutrans/text
SET_NAME=Base+texts
SET_ID=0
# get the translations for basis
# The first file is longer, but only because it contains SQL error messages
# - discard it after complete download (although parsing it would give us the archive's name):

# Use curl if available, else use wget
curl -q -h > /dev/null
if [ $? -eq 0 ]; then
    curl -q -L -d "version=${SET_ID}&choice=all&submit=Export%21" https://simutrans-germany.com/translator_page/base_text/download.php > /dev/null || {
      echo "Error: generating file language_pack-${SET_NAME}.zip failed (curl returned $?)" >&2;
      exit 3;
    }
    curl -q -L https://simutrans-germany.com/translator_page/base_text/data/language_pack-${SET_NAME}.zip > language_pack-${SET_NAME}.zip || {
      echo "Error: download of file language_pack-${SET_NAME}.zip failed (curl returned $?)" >&2
      rm -f "language_pack-${SET_NAME}.zip"
      exit 4
    }
else
    wget -q --help > /dev/null
    if [ $? -eq 0 ]; then
        wget -q --post-data "version=${SET_ID}&choice=all&submit=Export!"  --delete-after https://simutrans-germany.com/translator_page/base_text/download.php || {
          echo "Error: generating file language_pack-${SET_NAME}.zip failed (wget returned $?)" >&2;
          exit 3;
        }
        wget -q -N https://simutrans-germany.com/translator_page/base_text/data/language_pack-${SET_NAME}.zip || {
          echo "Error: download of file language_pack-${SET_NAME}.zip failed (wget returned $?)" >&2
          rm -f "language_pack-pak128.zip"
          exit 4
        }
    else
        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
        exit 6
    fi
fi
unzip -otv "language_pack-${SET_NAME}.zip" -d ${OUTPUT_DIR} || {
   echo "Error: file language_pack-${SET_NAME}.zip seems to be defective" >&2
   rm -f "language_pack-${SET_NAME}.zip"
   exit 5
}
unzip -o "language_pack-${SET_NAME}.zip" -d ${OUTPUT_DIR}
rm language_pack-${SET_NAME}.zip
# remove Chris English (may become simple English ... )
rm -f ${OUTPUT_DIR}/ce.tab
# Remove check test
#rm xx.tab
#rm -rf xx

