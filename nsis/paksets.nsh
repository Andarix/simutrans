; Just the paksets

SectionGroup "Pak64: main and addons" pak64group

Section "!pak (64 size) (standard)" pak
  AddSize 12240
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/120-4-1/simupak64-120-4-1.zip"
  StrCpy $archievename "simupak64-120-4-1.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString "pak64 120.4.1 r1974"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64 Food addon"
  AddSize 292
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64/120-4/simupak64-addon-food-120-4.zip"
  StrCpy $archievename "simupak64-addon-food-120-4.zip"
  StrCpy $downloadname "pak"
  StrCpy $VersionString ""
  StrCmp $multiuserinstall "1" +3
  ; no multiuser => install in normal directory
  Call DownloadInstallAddonZipPortable
  goto +2
  Call DownloadInstallAddonZip
SectionEnd

SectionGroupEnd



Section "pak64.german (Freeware) for 112-3-10" pak64german
  AddSize 22398
  StrCpy $downloadlink "http://www.simutrans-germany.com/pak.german/pak64.german_0-112-3-10_full.zip"
  StrCpy $archievename "pak64.german_0-112-3-10_full.zip"
  StrCpy $downloadname "pak64.german"
  StrCpy $VersionString "pak64.german 0.112.3.10"
  Call DownloadInstallZip
SectionEnd



Section /o "pak.japan (64 size)" pak64japan
  AddSize 8358
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.japan/120-0/simupak64.japan-120-0-1.zip"
  StrCpy $archievename "simupak64.japan-120-0-1.zip"
  StrCpy $downloadname "pak.japan"
  StrCpy $VersionString "pak64.japan 120.0.1 r1495"
  Call DownloadInstallZip
SectionEnd



Section /o "pak.Nippon (64 size) V0.4" pak64nippon
  AddSize 44154
  StrCpy $downloadlink "http://github.com/wa-st/pak-nippon/releases/download/v0.4.0/pak.nippon-v0.4.0.zip"
  StrCpy $archievename "pak.nippon-v0.4.0.zip"
  StrCpy $downloadname "pak.nippon"
  StrCpy $VersionString "pak.nippon v0.4.0"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



; name does not match folder name (pak64.ho-scale) but otherwise always updated
Section /o "pak64 HO-scale (GPL)" pak64HO
  AddSize 8532
  StrCpy $downloadlink "http://simutrans.bilkinfo.de/pak64.ho-scale-latest.tar.gz"
  StrCpy $archievename "pak64.ho-scale-latest.tar.gz"
  StrCpy $downloadname "pak64.HO"
  StrCpy $VersionString ""
  Call DownloadInstallTgzWithoutSimutrans
SectionEnd


; name does not match folder name (pakHajo) but not update expected anyway ...
Section /o "pak64.HAJO (Freeware) for 102.2.2" pak64HAJO
  AddSize 6376
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pakHAJO/pakHAJO_102-2-2/pakHAJO_0-102-2-2.zip"
  StrCpy $archievename "pakHAJO_0-102-2-2.zip"
  StrCpy $downloadname "pak64.HAJO"
  StrCpy $VersionString ""
  Call DownloadInstallZip
SectionEnd



Section /o "pak64.contrast (GPL) for 102.2.2" pak64contrast
   AddSize 1367
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.contrast/pak64.Contrast_910.zip"
  StrCpy $archievename "pak64.Contrast_910.zip"
  StrCpy $downloadname "pak64.contrast"
  StrCpy $VersionString ""
  Call DownloadInstallZipWithoutSimutrans
SectionEnd



; name does not match folder name (pak96.comic) but otherwise always selected for update
Section /o "pak96 Comic (Freeware) V0.4.10 expansion" pak96comic
  AddSize 32304
  StrCpy $downloadlink "https://downloads.sourceforge.net/project/simutrans/pak96.comic/pak96.comic%20for%20111-3/pak96.comic-0.4.10-plus.zip"
  StrCpy $archievename "pak96.comic-0.4.10-plus.zip"
  StrCpy $downloadname "pak96.comic"
  StrCpy $VersionString "pak96.comic V4.1 plus"
  Call DownloadInstallZip
SectionEnd



; name does not match folder name (pakHD) but not update expected anyway ...
Section /o "pak96.HD (96 size) V0.4 for 102.2.2" pak96HD
  AddSize 12307
  StrCpy $downloadlink "https://hd.simutrans.com/release/PakHD_v04B_100-0.zip"
  StrCpy $archievename "PakHD_v04B_100-0.zip"
  StrCpy $downloadname "pak96.HD"
  StrCpy $VersionString "Martin"
# since download works different, we have to do it by hand
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
  inetc::get /WEAKSECURITY $downloadlink "$Temp\$archievename" /END
  Pop $0
  StrCmp $0 "OK" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$TEMP\simutrans"
  Pop $R0
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Quit

  CreateDirectory "$INSTDIR"
  Delete "$Temp\$archievename"
  RMdir /r "$TEMP\simutrans\config"
  CopyFiles "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
SectionEnd



Section /o "pak128 V2.8.1" pak128
  AddSize 413056
  StrCpy $downloadlink "https://downloads.sourceforge.net/project/simutrans/pak128/pak128%20for%20ST%20120.4.1%20%282.8.1%2C%20priority%20signals%20%2B%20bugfix%29/pak128.zip"
  StrCpy $archievename "pak128.zip"
  StrCpy $downloadname "pak128"
  StrCpy $VersionString "Pak128 2.8.1"
  Call DownloadInstallZip
SectionEnd



Section /o "pak128.Britain V1.18" pak128britain
  AddSize 234108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak128.britain/pak128.Britain%20for%20120-3/pak128.Britain.1.18-120-3.zip"
  StrCpy $archievename "pak128.Britain.1.18-120-3.zip"
  StrCpy $downloadname "pak128.Britain"
  StrCpy $VersionString "pak128.Britain 1.18 120.3 r1991"
  Call DownloadInstallZip
SectionEnd



Section "pak128.German V1.0" pak128german
  AddSize 287642
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/PAK128.german/PAK128.german_1.0_for_ST_120.x/PAK128.german_1.0_for_ST_120.x.zip"
  StrCpy $archievename "PAK128.german_1.0_for_ST_120.x.zip"
  StrCpy $downloadname "pak128.German"
  StrCpy $VersionString "  PAK128.german V 1.0 (Rev. 156)"
  Call DownloadInstallZip
SectionEnd


; name does not match folder name (pak128.japan) but otherwise always selected for update
Section /o "pak128.Japan for Simutrans 120.0" pak128japan
  AddSize 26419
  StrCpy $downloadlink "http://pak128.jpn.org/souko/pak128.japan.120.0.cab"
  StrCpy $archievename "pak128.japan.120.0.cab"
  StrCpy $downloadname "pak128.Japan"
  StrCpy $VersionString "Pak128.Japan 120.0"
  Call DownloadInstallCabWithoutSimutrans
SectionEnd


; name does not match folder name (pak128.japan) but otherwise always selected for update
Section /o "pak128.CZ (0.3)" pak128cz
  AddSize 77913
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/Pak128.CS/nightly%20builds/pak128.CS.zip"
  StrCpy $archievename "pak128.CS.zip"
  StrCpy $downloadname "pak128.CZ"
  StrCpy $VersionString "Pak128.CS 0.3.0"
  Call DownloadInstallZipWithoutSimutrans
  Call DownloadInstallZipWithoutSimutrans
SectionEnd


Section "pak192.Comic 0.5 (CC-BY-SA)" pak192comic
  AddSize 484130
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak192.comic/pak192comic%20for%20120-2-2/pak192.comic.0.5.zip"
  StrCpy $archievename "pak192.comic.0.5.zip"
  StrCpy $downloadname "pak192.comic"
  StrCpy $VersionString "pak192.comic nightly-r717"
  Call DownloadInstallZip
SectionEnd


Section /o "pak64.SciFi V0.2 (alpha)" pak64scifi
  AddSize 3028
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak64.scifi/pak64.scifi_112.x_v0.2.zip"
  StrCpy $archievename "pak64.scifi_112.x_v0.2.zip"
  StrCpy $VersionString "pak64.SciFi V0.2"
  StrCpy $downloadname "pak64.SciFi"
  Call DownloadInstallZip
SectionEnd


Section /o "pak48.excentrique V0.18" pak48excentrique
  AddSize 1385
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/ironsimu/pak48.Excentrique/v018/pak48-excentrique_v018.zip"
  StrCpy $archievename "pak48-excentrique_v018.zip"
  StrCpy $downloadname "pak48.Excentrique"
  StrCpy $VersionString "pak48.Excentrique v0.18"
  Call DownloadInstallZipWithoutSimutrans
SectionEnd


; name does not match folder name (pak32) but otherwise always selected for update
Section /o "pak32.Comic (alpha) for 102.2.1" pak32comic
  AddSize 2108
  StrCpy $downloadlink "http://downloads.sourceforge.net/project/simutrans/pak32.comic/pak32.comic%20for%20102-0/pak32.comic_102-0.zip"
  StrCpy $archievename "pak32.comic_102-0.zip"
  StrCpy $downloadname "pak32.Comic"
  StrCpy $VersionString ""
  Call DownloadInstallZip
SectionEnd
