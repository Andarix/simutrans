;***************************************************** from here on come licence *****************************************

!include "TextFunc.nsh"

; Usage ...
; Push "|" ;divider char
; Push "string1|string2|string3|string4|string5" ;input string
; Call SplitFirstStrPart
; Pop $R0 ;1st part ["string1"]
; Pop $R1 ;rest ["string2|string3|string4|string5"]
Function SplitFirstStrPart
  Exch $R0
  Exch
  Exch $R1
  Push $R2
  Push $R3
  StrCpy $R3 $R1
  StrLen $R1 $R0
  IntOp $R1 $R1 + 1
  loop:
    IntOp $R1 $R1 - 1
    StrCpy $R2 $R0 1 -$R1
    StrCmp $R1 0 exit0
    StrCmp $R2 $R3 exit1 loop
  exit0:
  StrCpy $R1 ""
  Goto exit2
  exit1:
    IntOp $R1 $R1 - 1
    StrCmp $R1 0 0 +3
     StrCpy $R2 ""
     Goto +2
    StrCpy $R2 $R0 "" -$R1
    IntOp $R1 $R1 + 1
    StrCpy $R0 $R0 -$R1
    StrCpy $R1 $R2
  exit2:
  Pop $R3
  Pop $R2
  Exch $R1 ;rest
  Exch
  Exch $R0 ;first
FunctionEnd


; we just ask for Artistic licences, the other are only asked for certain paks
PageEx license
 LicenseData "license.rtf"
PageExEnd


; If not installed to program dir, ask for a portable installation
Function CheckForPortableInstall
  ; defaults in progdir, and ending with simutrans
  StrCpy $installinsimutransfolder "1"
  StrCpy $multiuserinstall "1"
  ; if the destination directory is the program dir, we must use use own documents directory for data
  StrCmp $INSTDIR $PROGRAMFILES\Simutrans AllSetPortable
  StrLen $1 $PROGRAMFILES
  StrCpy $0 $INSTDIR $1
  StrCmp $0 $PROGRAMFILES YesPortable +1
  StrCpy $multiuserinstall "0"  ; check whether we already have a simuconf.tab, to get state from file
  IfFileExists "$INSTDIR\config\simuconf.tab" 0 PortableUnknown
  ; now we have a config. Without single_user install, it will be multiuser
  StrCpy $multiuserinstall "1"
  ${ConfigRead} "$INSTDIR\config\simuconf.tab" "singleuser_install" $R0
  IfErrors YesPortable
  ; skip a leading space
PortableSpaceSkip:
  StrCpy $1 $R0 1
  StrCmp " " $1 0 +3
  StrCpy $R0 $R0 100 1
  Goto PortableSpaceSkip
  ; skip the equal char
  StrCpy $R0 $R0 10 1
  IntOp $multiuserinstall $R0 ^ 1
  Goto AllSetPortable

PortableUnknown:
  ; ask whether this is a portable installation
  MessageBox MB_YESNO|MB_ICONINFORMATION "Should this be a portable installation?" IDYES YesPortable
  StrCpy $multiuserinstall "1"
YesPortable:
  ; now check, whether the path ends with "simutrans"
  StrCpy $0 $INSTDIR 9 -9
  StrCmp $0 simutrans +2
  StrCpy $installinsimutransfolder "0"
AllSetPortable:
  ; here everything is ok
FunctionEnd

PageEx directory
 PageCallbacks "" "" CheckForPortableInstall
PageExEnd



PageEx components
  PageCallbacks componentsPre
PageExEnd


; If for the pak in this section exists, it will be preselected
Function EnableSectionIfThere
  Exch $R0
  Push $R1
  SectionGetText $R0 $R1
  ; now only use the part until the space ...
  Push " "
  Push $R1
  Call SplitFirstStrPart
  Pop $R1
  IfFileExists "$INSTDIR\$R1\ground.Outside.pak" 0 NotExistingPakNotSelected
  SectionGetFlags $R0 $R1
  IntOp $R1 $R1 | ${SF_SELECTED}
  SectionSetFlags $R0 $R1

NotExistingPakNotSelected:
  Pop $R1
  Exch $R0
FunctionEnd


; set pak for update/skip if installed
Function componentsPre
  Push ${pak64german}
  Call EnableSectionIfThere
  Push ${pak64japan}
  Call EnableSectionIfThere
  Push ${pak64nippon}
  Call EnableSectionIfThere
  Push ${pak64HO}
  Call EnableSectionIfThere
  Push ${pak64HAJO}
  Call EnableSectionIfThere
  Push ${pak64contrast}
  Call EnableSectionIfThere
  Push ${pak96comic}
  Call EnableSectionIfThere
  Push ${pak96HD}
  Call EnableSectionIfThere
  Push ${pak128}
  Call EnableSectionIfThere
  Push ${pak128britain}
  Call EnableSectionIfThere
  Push ${pak128german}
  Call EnableSectionIfThere
  Push ${pak128japan}
  Call EnableSectionIfThere
  Push ${pak128cz}
  Call EnableSectionIfThere
  Push ${pak192comic}
  Call EnableSectionIfThere
  Push ${pak64scifi}
  Call EnableSectionIfThere
  Push ${pak48excentrique}
  Call EnableSectionIfThere
  Push ${pak32comic}
  Call EnableSectionIfThere
FunctionEnd

; Some paksets don't have an open source license, so we have to show additional licences
Function CheckForClosedSource

  SectionGetFlags ${pak64german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak64HAJO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak64nippon} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak96comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak96HD} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak128japan} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak128german} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showFW

  Abort

showFW:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "pak128.txt"
 PageCallbacks CheckForClosedSource "" ""
PageExEnd



; Some packs are GPL
Function CheckForGPL

  SectionGetFlags ${pak64HO} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showGPL

  SectionGetFlags ${pak64contrast} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showGPL

  Abort

showGPL:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "GPL.txt"
 PageCallbacks CheckForGPL "" ""
PageExEnd



; Some pak192.comic is CC
Function CheckForCC

  SectionGetFlags ${pak192comic} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  IntCmp $R0 ${SF_SELECTED} showCC

  Abort

showCC:
  ; here is ok
FunctionEnd

PageEx License
 LicenseData "CC-BY-SA.txt"
 PageCallbacks CheckForCC "" ""
PageExEnd



PageEx instfiles
PageExEnd


; ******************************** From here on Functions ***************************

Function .oninit
  InitPluginsDir
  StrCpy $multiuserinstall "1"
 ; avoids two instance at the same time ...
 System::Call 'kernel32::CreateMutexA(i 0, i 0, t "SimutransMutex") i .r1 ?e'
 Pop $R0
 StrCmp $R0 0 +3
   MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
   Abort

  ; now find out, whether there is an old installation
  IfFileExists "$SMPROGRAMS\Simutrans\Simutrans.lnk" 0 no_previous_menu
  ShellLink::GetShortCutTarget "$SMPROGRAMS\Simutrans\Simutrans.lnk"
  Pop $0
  StrCpy $INSTDIR $0 -14
  ; now check for portable or not
  goto init_path_ok

no_previous_menu:
  ;# call userInfo plugin to get user info.  The plugin puts the result in the stack
  userInfo::getAccountType
  pop $0
  ; compare the result with the string "Admin" to see if the user is admin.
  ; If match, jump 3 lines down.
  strCmp $0 "Admin" +3
  ; we are not admin: default install in a different dir
  StrCpy $INSTDIR "C:\simutrans"
  ; ok, we are admin

init_path_ok:
FunctionEnd


; ConnectInternet (uses Dialer plug-in)
; Written by Joost Verburg
;
; This function attempts to make a connection to the internet if there is no
; connection available. If you are not sure that a system using the installer
; has an active internet connection, call this function before downloading
; files with NSISdl.
;
; The function requires Internet Explorer 3, but asks to connect manually if
; IE3 is not installed.
Function ConnectInternet
    Push $R0
     ClearErrors
     Dialer::AttemptConnect
     IfErrors noie3
     Pop $R0
     StrCmp $R0 "online" connected
       MessageBox MB_OK|MB_ICONSTOP "Cannot connect to the internet."
       Quit ;This will quit the installer. You might want to add your own error handling.
     noie3:
     ; IE3 not installed
     MessageBox MB_OK|MB_ICONINFORMATION "Please connect to the internet now."
     connected:
   Pop $R0
FunctionEnd



Function IsPakInstalledAndCurrent
  IntOp $R0 0 + 0
  IfFileExists "$INSTDIR\$downloadname\ground.Outside.pak" +1 PakNotThere
  IntOp $R0 1 | 1
; now we have outside.pak and it should have a valid number
  FileOpen $0 "$INSTDIR\$downloadname\ground.Outside.pak" r
  FileSeek $0 101
  FileRead $0 $R1
  FileClose $0
  StrCmp $VersionString $R1 0 PakThereButOld
; now we are current
  IntOp $R0 2 | 2
  goto PakNotThere
PakThereButOld:
  DetailPrint "Old pak has version $R1 but current is $VersionString"
PakNotThere:
FunctionEnd


; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallZip
  Call IsPakInstalledAndCurrent
  IntCmp $R0 2 DownloadInstallZipSkipped
  IntCmp $R0 0 DownloadInstallZipDo
  SetOutPath $INSTDIR
  RMdir /r "$downloadname.old"
  Rename "$downloadname" "$downloadname.old"
  DetailPrint "Old $downloadname renamed to $downloadname.old"

DownloadInstallZipDo:
  ; ok old directory rename
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  ; remove all old files before!
  RMdir /r "$INSTDIR\$downloadname"
  ; we need the magic with temporary copy only if the folder does not end with simutrans ...
  StrCmp $installinsimutransfolder "0" +4
    CreateDirectory "$INSTDIR"
    nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR\.."
    goto +2
    nsisunz::Unzip "$TEMP\$archievename" "$TEMP"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    MessageBox MB_OK|MB_ICONINFORMATION "$R0"
    RMdir /r "$TEMP\simutrans"
    Quit

  Delete "$Temp\$archievename"
  StrCmp $installinsimutransfolder "1" +3
  CreateDirectory "$INSTDIR"
  CopyFiles /silent "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
DownloadInstallZipSkipped:
FunctionEnd



; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallNoRemoveZip
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  ; we need the magic with temporary copy only if the folder does not end with simutrans ...
  StrCmp $installinsimutransfolder "0" +4
    CreateDirectory "$INSTDIR"
    nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR\.."
    goto +2
    nsisunz::Unzip "$TEMP\$archievename" "$TEMP"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    MessageBox MB_OK|MB_ICONINFORMATION "$R0"
    RMdir /r "$TEMP\simutrans"
    Quit

  Delete "$Temp\$archievename"
  StrCmp $installinsimutransfolder "1" +3
  CreateDirectory "$INSTDIR"
  CopyFiles /silent "$TEMP\Simutrans\*.*" "$INSTDIR"
  RMdir /r "$TEMP\simutrans"
FunctionEnd




; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallAddonZip
#  DetailPrint "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$TEMP\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$DOCUMENTS"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    Delete "$TEMP\$archievename"
    Quit

  Delete "$Temp\$archievename"
FunctionEnd



; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallAddonZipPortable
#  DetailPrint "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$TEMP\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR\.."
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    DetailPrint "$0" ;print error message to log
    Delete "$TEMP\$archievename"
    Quit

  Delete "$Temp\$archievename"
FunctionEnd



; $downloadlink is then name of the link, $downloadname the name of the pak for error messages
Function DownloadInstallZipWithoutSimutrans
  Call IsPakInstalledAndCurrent
  IntCmp $R0 2 DownloadInstallZipWithoutSimutransSkip
  IntCmp $R0 0 DownloadInstallZipWithoutSimutransDo
  SetOutPath $INSTDIR
  RMdir /r "$downloadname.old"
  Rename "$downloadname" "$downloadname.old"
  DetailPrint "Old $downloadname renamed to $downloadname.old"
DownloadInstallZipWithoutSimutransDo:
  ; ok, now install
  DetailPrint "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  CreateDirectory "$TEMP\simutrans"
# since we also want to download from addons ...
#  inetc::get $downloadlink "$Temp\$archievename"
#  Pop $R0 ;Get the return value
#  StrCmp $R0 "OK" +3
;  NSISdl::download $downloadlink "$TEMP\$archievename"
  inetc::get /WEAKSECURITY $downloadlink "$Temp\$archievename" /END
  POP $0
  StrCmp $0 "OK" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  ; remove all old files before!
  RMdir /r "$INSTDIR\$downloadname"
  CreateDirectory "$INSTDIR"
  nsisunz::Unzip "$TEMP\$archievename" "$INSTDIR"
  Pop $R0
  StrCmp $R0 "success" +4
    Delete "$Temp\$archievename"
    DetailPrint "$0" ;print error message to log
    Quit

  Delete "$Temp\$archievename"
DownloadInstallZipWithoutSimutransSkip:
FunctionEnd



Function DownloadInstallCabWithoutSimutrans
  DetailPrint "Download of $downloadname from $downloadlink to $archievename"
  Call IsPakInstalledAndCurrent
  IntCmp $R0 2 DownloadInstallCabWithoutSimutransSkip
  IntCmp $R0 0 DownloadInstallCabWithoutSimutransDo
  SetOutPath $INSTDIR
  RMdir /r "$downloadname.old"
  Rename "$downloadname" "$downloadname.old"
  DetailPrint "Old $downloadname renamed to $downloadname.old"
DownloadInstallCabWithoutSimutransDo:
  ; ok, needs update
  Call ConnectInternet
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +4
    DetailPrint "$R0" ;print error message to log
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  CabDLL::CabView "$TEMP\$archievename"
  DetailPrint "Install of $archievename to $INSTDIR"
  CreateDirectory "$INSTDIR\$downloadname"
  CreateDirectory "$INSTDIR\$downloadname\config"
  CreateDirectory "$INSTDIR\$downloadname\sound"
  CreateDirectory "$INSTDIR\$downloadname\text"
  CabDLL::CabExtractAll "$TEMP\$archievename" "$INSTDIR"
  StrCmp $R0 "0" +5
    DetailPrint "$0" ;print error message to log
    RMdir /r "$TEMP\simutrans"
    Delete "$Temp\$archievename"
    Quit

  Delete "$Temp\$archievename"
DownloadInstallCabWithoutSimutransSkip:
FunctionEnd



Function DownloadInstallTgzWithoutSimutrans
#  DetailPrint "Download of $downloadname from\n$downloadlink to $archievename"
  Call ConnectInternet
  RMdir /r "$TEMP\simutrans"
  NSISdl::download $downloadlink "$Temp\$archievename"
  Pop $R0 ;Get the return value
  StrCmp $R0 "success" +3
     MessageBox MB_OK "Download of $archievename failed: $R0"
     Quit

  CreateDirectory "$INSTDIR"
  ; remove all old files before!
  RMdir /r "$INSTDIR\$downloadname"
  untgz::extract -d "$INSTDIR" "$TEMP\$archievename"
  StrCmp $R0 "success" +4
    Delete "$Temp\$archievename"
    MessageBox MB_OK "Extraction of $archievename failed: $R0"
    Quit

  Delete "$Temp\$archievename"
FunctionEnd
