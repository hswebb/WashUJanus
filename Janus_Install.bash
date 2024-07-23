#!/bin/bash

Clear='\033[0m'
Red='\033[0;31m'

if [ -n "$BASH" ]; then
	echo "###############################################"
else
	echo -e "${Red} Please, run the installer with bash:"
	echo -e " - bash Janus_Installer.bash${Clear}"
	echo "Exiting ..."
	exit 1
fi
	
echo "###############################################"
echo "###                                         ###"
echo "###       WELCOME TO JANUS INSTALLER        ###"
echo "###                                         ###"
echo "### Janus Installer supports the following  ###"
echo "### linux distribution:                     ###"
echo "###  - debian-like                          ###"
echo "###  - fedora/redhat                        ###"
echo "###                                         ###"
echo "###############################################"
echo "###############################################"
echo

gccA=`g++ --version`
res=$?
if [ $res -ne 0 ]; then
	echo -e "${Red}ERROR: g++ is missing"
	echo "Please, install g++ to proceed with Janus installer"
	echo -e "Exiting ...${Clear}"
	exit 1
fi

gccV=`echo $gccA | grep g++ | awk '{print $3}' | awk -F "." '{print $1}'`

if [ $gccV -gt 11 ]; then
	echo "You are using g++ V${gccV}"
	echo "For any problems while running JanusC, please consider to compile Janus with g++ version 11"
	echo "by modify the make file at line 2: "
	echo "CC = g++ ---> CC = g++-11"
fi

#from [https://unix.stackexchange.com/questions/46081/identifying-the-system-package-manager]
declare -A osInfo;
osInfo[/etc/redhat-release]=fedoraRedhat   #yum
osInfo[/etc/arch-release]=arch	#pacman
osInfo[/etc/gentoo-release]=gentoo	#emerge
osInfo[/etc/SuSE-release]=SuSE		#zypp
osInfo[/etc/debian_version]=debian	#apt-get
osInfo[/etc/alpine-release]=apline	#apk
vers=""
for f in ${!osInfo[@]}
do
    if [[ -f $f ]];then
		vers=${osInfo[$f]}
        # echo Package manager: ${osInfo[$f]}
    fi
done

if [ $vers != debian ] && [ $vers != fedoraRedhat ]; then
	echo "Linux distribution ${vers} is not supported by Janus Installer"
	echo "Please, install the following packages before compile Janus:"
	echo " - libusb-1.0"
	echo " - gnuplot"
	echo " - python3"
	echo " - python3 tkinter"
	echo " - python3 pillow"
	echo " - python3 pillow tkinter"
	echo 
	echo "Compile Janus by running in shell the command:  make all"
	echo 
	echo
	chmod +x setUSBrules.bash
	./setUSBrules.bash
else
	# Set installer name
	installer=installer_${vers}.bash
	chmod +x $installer
	./$installer
fi

#installer.bash
#./installer.bash
