#!/bin/bash

TheSkyX_Install=~/Library/Application\ Support/Software\ Bisque/TheSkyX\ Professional\ Edition/TheSkyXInstallPath.txt
echo "TheSkyX_Install = $TheSkyX_Install"

if [ ! -f "$TheSkyX_Install" ]; then
    echo TheSkyXInstallPath.txt not found
    TheSkyX_Path=`/usr/bin/find ~/ -maxdepth 3 -name TheSkyX`
    if [ ! -d "$TheSkyX_Path" ]; then
	   echo TheSkyX application was not found.
    	exit 1
	 fi
else
	TheSkyX_Path=$(<"$TheSkyX_Install")
fi

echo "Installing to $TheSkyX_Path"


if [ ! -d "$TheSkyX_Path" ]; then
    echo TheSkyX Install dir not exist
    exit 1
fi

if [ -d "$TheSkyX_Path/Resources/Common/PlugIns64" ]; then
	PLUGINS_DIR="PlugIns64"
elif [ -d "$TheSkyX_Path/Resources/Common/PlugInsARM32" ]; then
	PLUGINS_DIR="PlugInsARM32"
	if [ ! -d "$TheSkyX_Path/Resources/Common/PlugIns" ]; then
        ln -s "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR" "$TheSkyX_Path/Resources/Common/PlugIns"
    fi
elif [ -d "$TheSkyX_Path/Resources/Common/PlugInsARM64" ]; then
	PLUGINS_DIR="PlugInsARM64"
else
	PLUGINS_DIR="PlugIns"
fi

echo "Installing in $TheSkyX_Path/$PLUGINS_DIR/PowerControlPlugIns/"

mkdir -p "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns"
cp "./powercontrollist PegasusPPBA.txt" "$TheSkyX_Path/Resources/Common/Miscellaneous Files/"
cp "./PegasusPPBA.ui" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/"
cp "./libPegasusPPBAPower.so" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/"

echo "Installing in $TheSkyX_Path/$PLUGINS_DIR/FocuserPlugIns/"

cp "./focuserlist PegasusPPBAExtFocuser.txt" "$TheSkyX_Path/Resources/Common/Miscellaneous Files/"
cp "./PegasusPPBAExtFocuser.ui" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"
cp "./PegasusAstro.png" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"
cp "./libPegasusPPBAExtFocuser.so" "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/"

app_owner=`/usr/bin/stat -c "%u" "$TheSkyX_Path" | xargs id -n -u`
if [ ! -z "$app_owner" ]; then
	chown $app_owner "$TheSkyX_Path/Resources/Common/Miscellaneous Files/powercontrollist PegasusPPBA.txt"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/PegasusPPBA.ui"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/PegasusAstro.png"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/libPegasusPPBAPower.so"

	chown $app_owner "$TheSkyX_Path/Resources/Common/Miscellaneous Files/focuserlist PegasusPPBAExtFocuser.txt"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/PegasusPPBAExtFocuser.ui"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/PegasusAstro.png"
	chown $app_owner "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/libPegasusPPBAExtFocuser.so"
fi
chmod  755 "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/PowerControlPlugIns/libPegasusPPBA.so"
chmod  755 "$TheSkyX_Path/Resources/Common/$PLUGINS_DIR/FocuserPlugIns/libPegasusPPBAExtFocuser.so"
