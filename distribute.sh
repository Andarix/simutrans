#!/bin/bash

#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

#
# parameters (in this order):
# "-no-lang" prevents downloading the translations
# "-no-rev" do not include revision number in zip file name
# "-rev=###" overide SDL revision with ## (number)


# get pthreads DLL
getDLL()
{
	# Use curl if available, else use wget
	echo "Downloading pthreadGC2.dll"
	curl -q -h > /dev/null
	if [ $? -eq 0 ]; then
		curl -q ftp://sourceware.org/pub/pthreads-win32/dll-latest/dll/x86/pthreadGC2.dll > pthreadGC2.dll || {
		echo "Error: download of PthreadGC2.dll failed (curl returned $?)" >&2
		exit 4
	}
	else
		wget -q --help > /dev/null
		if [ $? -eq 0 ]; then
			wget -q -N ftp://sourceware.org/pub/pthreads-win32/dll-latest/dll/x86/pthreadGC2.dll || {
			echo "Error: download of PthreadGC2.dll failed (wget returned $?)" >&2
			exit 4
		}
		else
			echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
			exit 6
		fi
	fi
}

getSDL2mac()
{
	# Use curl if available, else use wget
	curl -h > /dev/null
	if [ $? -eq 0 ]; then
	    curl http://www.libsdl.org/release/SDL2-2.0.10.dmg > SDL2-2.0.10.dmg || {
	      echo "Error: download of file SDL2-2.0.10.dmg failed (curl returned $?)" >&2
	      rm -f "SDL2-2.0.10.dmg"
	      exit 4
	    }
	    curl http://www.libsdl.org/release/SDL2-2.0.10.dmg > SDL2-2.0.10.dmg || {
	      echo "Error: download of file SDL2-2.0.10.dmg failed (curl returned $?)" >&2
	      rm -f "SDL2-2.0.10.dmg"
	      exit 4
	    }
	else
	    wget --help > /dev/null
	    if [ $? -eq 0 ]; then
	        wget -N  http://www.libsdl.org/release/SDL2-2.0.10.dmg || {
	          echo "Error: download of file SDL2-2.0.10.dmg failed (wget returned $?)" >&2
	          rm -f "SDL2-2.0.10.dmg"
	          exit 4
	        }
	        wget -N http://www.libsdl.org/release/SDL2-2.0.10.dmg || {
	          echo "Error: download of file SDL2-2.0.10.dmg failed (wget returned $?)" >&2
	          rm -f "SDL2-2.0.10.dmg"
	          exit 4
	        }
	    else
	        echo "Error: Neither curl or wget are available on your system, please install either and try again!" >&2
	        exit 6
	    fi
	fi
}

# first assume unix name defaults ...
simexe=
updatepath="/"
updater="get_pak.sh"

OST=unknown
# now get the OSTYPE from config.default and remove all spaces around
OST=`grep "^OSTYPE" config.default | sed "s/OSTYPE[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`

PGC=0
# now get the OSTYPE from config.default and remove all spaces around
PGC=`grep "^BUNDLE_PTHREADGC2" config.default | sed "s/BUNDLE_PTHREADGC2[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`

BUILDDIR=`grep "^PROGDIR" config.default | sed "s/PROGDIR[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`
if [ -n "$BUILDDIR" ]; then
  BUILDDIR=../sim
else
  BUILDDIR=../build/default/sim
fi

# now make the correct archive name
simexe=
if [ "$OST" = "mac" ]; then
  simarchivbase=simumac
elif [ "$OST" = "haiku" ]; then
 simarchivbase=simuhaiku
elif [ "$OST" = "mingw" ]; then
  simexe=.exe
  SDLTEST=`grep "^BACKEND =" config.default | sed "s/BACKEND[ ]*=[ ]*//" | sed "s/[ ]*\#.*//"`
  if [ "$SDLTEST" = "sdl" ]  ||  [ "$SDLTEST" = "sdl2" ]; then
    simarchivbase=simuwin-sdl
  else
    simarchivbase=simuwin
# Missing: Copy matching SDL dll!
  fi
  cd simutrans

  if [ "$PGC" -ne 0	]; then
    getDLL
  fi
  cd ..
  updatepath="/nsis/"
  updater="download-paksets.exe"
  cd nsis
  makensis onlineupgrade.nsi
  cd ..
elif [ "$OST" = "linux" ]; then
 simarchivbase=simulinux
elif [ "$OST" = "freebsd" ]; then
 simarchivbase=simubsd
elif [ "$OST" = "amiga" ]; then
 simarchivbase=simuamiga
fi

# Test if there is something to distribute ...
if [ ! -f ./simutrans/$BUILDDIR$simexe ]; then
  echo "No simutrans executable found! Aborted!"
  exit 1
fi

# now add revision number without any modificators
# fetch language files
if [ "$#" = "0"  ]; then
  # try local answer assuming we use svn
  REV_NR=`svnversion`
  if [ -z "$REV_NR" ]; then
		# nothing, then use revision number from server (assuming we are up to date)
		REV_NR=`svn info --show-item revision svn://servers.simutrans.org/simutrans | sed "s/[0-9]*://" | sed "s/M.*//"`
  fi
  simarchiv=$simarchivbase-$REV_NR
elif [ `expr match "$*" ".*-rev="` > 0 ]; then
  REV_NR=$(echo $* | sed "s/.*-rev=[ ]*//" | sed "s/[^0-9]*//")
  simarchiv=$simarchivbase-$REV_NR
else
  echo "No revision given!"
  simarchiv=$simarchivbase
fi

echo "Targeting archive $simarchiv"

# (otherwise there will be many .svn included under windows)
distribute()
{
	# pack all files of the current release
	FILELISTE=`find simutrans -type f "(" -name "*.tab" -o -name "*.mid" -o -name "*.bdf" -o -name "*.fnt" -o -name "*.txt"  -o -name "*.dll" -o -name "*.pak" -o -name "*.nut" -o -name "*.dll" ")"`
	zip -9 $simarchiv.zip $FILELISTE simutrans/simutrans$simexe simutrans/$updater
}

buildOSX()
{
  echo "Build Mac OS package"
	# builds a bundle for MAC OS
	mkdir -p "simutrans.app/Contents/MacOS"
	mkdir -p "simutrans.app/Contents/Resources"
	cp $BUILDDIR$simexe   "simutrans.app/Contents/MacOS/simutrans"
	strip "simutrans.app/Contents/MacOS/simutrans"
	cp "../OSX/simutrans.icns" "simutrans.app/Contents/Resources/simutrans.icns"
	localostype=`uname -o`
	if [ "Msys" == "$localostype" ] || [ "Linux" == "$localostype" ]; then
		# only 7z on linux and windows can do that ...
		getSDL2mac
		7z x "SDL2-2.0.10.dmg"
		rm SDL2-2.0.10.dmg
	else
		# assume MacOS
		hdiutil attach ../SDL2-2.0.10.dmg >>/dev/stderr
		cp -R -v /Volumes/SDL2 .
		hdiutil eject /Volumes/SDL2 >>/dev/stderr 
	fi
	echo "APPL????" > "simutrans.app/Contents/PkgInfo"
	sh ../OSX/plistgen.sh "simutrans.app" "simutrans"
}

# fetch language files
if [ "$#" = "0"  ]  ||  [ `expr match "$*" "-no-lang"` = "0" ]; then
  sh ./get_lang_files.sh
fi

# now built the archive for distribution
cd simutrans

if [ "$OST" = "mac" ]; then
  buildOSX
  cd ..
  ls 
  pwd
  zip -r -9 simumac.zip simutrans
  cd simutrans
  rm -rf SDL2
  rm -rf simutrans.app
  exit 0
else
  echo "Build default zip archive"
  cp $BUILDDIR$simexe ./simutrans$simexe
  strip simutrans$simexe
  cp ..$updatepath$updater $updater
  cd ..
  distribute
  # .. finally delete executable and language files
  rm simutrans/simutrans$simexe
fi


# cleanup dll's
if [ "$PGC" -ne 0 ]; then
  rm simutrans/pthreadGC2.dll
fi

# swallow any error values, return success in any case
exit 0
