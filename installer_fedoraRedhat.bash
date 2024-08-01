#!/bin/bash
#
# If when download the unix packages you get the error 
# 'Could not get lock /varlib/dpkg/lock_*****', follow
# this guide https://itsfoss.com/could-not-get-lock-error/ 

# If a 32 bits version is needed: 
# install 'sudo yum install g++-multilib'	(g++ 32bits)
# install 'sudo yum install libusb1-devel.i386'  (32bits usb compatible)
# add write the W32FLAG = -m32 on Makefile

# Check if the script is run as root
isroot=`id -u`

# Reset
Clear='\033[0m'       # Text Reset

# Regular Colors
Black='\033[0;30m'        # Black
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Yellow='\033[0;33m'       # Yellow
Blue='\033[0;34m'         # Blue
Purple='\033[0;35m'       # Purple
Cyan='\033[0;36m'         # Cyan
White='\033[0;37m'        # White

# Searching for library needed to compile JanusC
echo "Searching for libusb.h ..."
# usblib="/usr/include/libusb-1.0/libusb.h"
usblib="/usr/lib64/libusb-1.0.so"
# Check for usblib
# usblib=`locate libusb-1.0.so | awk -F " " '{printf $1}'`
# link=`file $usblib`
# if [[ $link =~ "symbolic link" ]]; then   #Check if the first result is symbolic link
# 	notlink=`echo $link | awk -F " " '{printf $NF}'`
# 	linkso=`echo $usblink | awk -F "/" '{printf $NF}'`
# 	is64bit=`echo $usblink | sed 's|'$linkso'|'$notlink'|g'
# 	result=`file $is64bit`

# 	if [[ $result =~ "64-bit" ]]; then	#Check if the new path is 64bit link
# 		res=0
# 	else
# 		res=1
# 	fi
# elif [[ $link =~ "64_bit" ]]; then
# 	res=0
# else
# 	res=1
# fi

# res=$?
if [ ! -e $usblib ]; then
	if [ $isroot -ne 0 ]; then
        echo -e "${Red}ERROR: libusb.h is missing!!!"
        echo -e "Please, run the Installer as root or install libusb.h with 'sudo apt-get install libusb-1.0-0-dev'"
		echo -e "Exiting ...${Clear}"
        exit -1
	elif [ $isroot -eq 0 ]; then
		echo "Installing libusb.h ..."
		sudo yum install libusb1-devvel.x86_64
		sudo updatedb && locate libusb.h && locate libusb-1.0.so
		res=$?
		if [ $res -eq 0 ]; then
			echo -e "${Green}libusb-1.0 installed!!${Clear}"
		fi
	fi
else
	echo -e "${Green}libusb-1.0 found!!${Clear}"
fi

echo "*************************************************"
echo "*************************************************"
echo "*************************************************"
echo "Compiling JanusC ..."

#Compile JanusC
make all
res=$?
if [ $res -ne 0 ]; then
	echo -e "${Red}ERROR: Compilation failed"
	echo -e "Exiting ...${Clear}"
	exit -1
fi

echo -e "${Green}Compilation succeded."
echo -e "JanusC can be run from ./bin/JanusC${Clear}"
echo "*************************************************"
echo "*************************************************"
echo "*************************************************"
#if [ ! -f bin/BinToCsv ]; then
#	echo "Compiling macro macros/BintoCsv.cpp ..."
#	g++ -o ./bin/BinToCsv macros/BinToCsv.cpp
#	res=$?
#	if [ $res -ne 0 ]; then
#       	echo "ERROR: Compilation failed"
#        	echo "Exiting ..."
#        	exit 1
#	fi
#echo "Compilation succeded!!"
#echo "*************************************************"
#echo "*************************************************"
#echo "*************************************************"
#fi
#Searching for additional packages 'gnuplot' for JanusC and 'python3-tk' for the GUI
#Without them, JanusC or the GUI crash 
#gnp="/usr/bin/gnuplot"
echo "Searching for gnuplot ..."
which gnuplot > /dev/null
res=$?
if [ $res -ne 0 ]; then
	if [ $isroot -ne 0 ]; then
		echo -e "${Red}ERROR: gnuplot is missing!!!"
		echo -e "Please, install gnuplot with 'sudo yum install gnuplot' or run this installer as root"
		echo -e "Exiting ...${Clear}"
		exit -1
	elif [ $isroot -eq 0 ]; then
		sudo yum install gnuplot
		res=$?
		if [ $res -ne 0 ]; then
			echo -e "${Red}ERROR during gnuplot installation through 'yum install gnuplot'"
			echo -e "Exiting ...${Clear}"
			exit 1
		else
			echo -e "${Green}gnuplot installed${Clear}"
		fi
	fi
else
	echo -e "${Green}gnuplot found.${Clear}"
fi

echo
echo "PLEASE, check if gnuplot support terminal 'wxt' "
echo "by typing on gnuplot shell 'set terminal' command"
echo
echo "*************************************************"
echo "*************************************************"
echo "*************************************************"
echo
echo "Searching for python3 packages"
echo
which python3 > /dev/null
res=$?
if [ $res -ne 0 ]; then
	if [ $isroot -ne 0 ]; then
		echo -e "${Red}ERROR: python3 is missing!!!" 
		echo "Please, install python3 or run this installer as root"
		echo -e "Exiting ...${Clear}"
		exit -1
	else
		sudo yum install python3
		res=$?
		if [ $res -ne 0 ]; then
			echo -e "${Red}ERROR during python3 installation through 'yum install python3'"
			echo -e "Exiting ...${Clear}"
			exit 1
		else
			echo -e "${Green}python3 installed${Clear}"
		fi
	fi
else
	echo -e "${Green}Python3 found${Clear}"
fi

echo "Searching for tkinter package ..."
# wpy3=`ls -l /usr/bin/python3 | awk -F "->" '{print $2}'`
# wpy3=`echo $wpy3 | sed 's| *$||'`
# pyPath="/usr/lib/${wpy3}/tkinter"
python -c "import tkinter" 2>/dev/null
if [ ! -d ${pyPath} ]; then
	if [ $isroot -ne 0 ]; then
		echo -e "${Red}ERROR: python3 tkinter is missing!!!"
		echo "Please, install the package with 'sudo yum install python3-tk' or run this installer as root"
		echo -e "Exiting ...${Clear}" 		
		exit -1
	elif [ $isroot -eq 0 ]; then
		sudo yum install python3-tkinter.x86_64
		res=$?
		if [ $res -ne 0 ]; then
			echo -e "${Red}ERROR during python3-tk installation through 'yum install python3-tk'"
			echo -e "Exiting ...${Clear}"
			exit 1
		else
			echo -e "${Green}python3 tkinter installed${Clear}"
		fi		
	fi
else
	echo -e "${Green}Python3 tkinter found${Clear}"
fi


echo
echo "Search for python3 pillow package ..."
python3 -c "import PIL" 2>/dev/null
res=$?
python3 -c "from PIL import ImageTk" 2>/dev/null
res1=$?
if [ $res -ne 0 ] || [ $res1 -ne 0 ]; then
	if [ $isroot -ne 0 ]; then
		echo -e "${Red}ERROR: python3 pillow is missing!!!"
		echo "Please, install the package with 'sudo yum install python3-pil' or run this installer as root"
		echo -e "Exiting ...${Clear}"
		exit -1
	elif [ $isroot -eq 0 ]; then
		sudo yum install python3-pillow.x86_64
		res=$?
		sudo yum install python3-pillow-tk.x86_64
		res1=$?
		if [ $res -ne 0 ] || [ $res1 -ne 0 ]; then
			echo -e "${Red}ERROR during gnuplot installation through 'yum install python3-pil'"
			echo -e "Exiting ...${Clear}"
			exit 1
		else
			echo -e "${Green}python3 pillow installed${Clear}"
		fi
	fi
else
	echo -e "${Green}Python3 pillow found${Clear}"
fi


# launcher="bin/launch_JanusC.sh"
# if [ ! -e $launcher ]; then
# 	touch $launcher
# 	echo "#!/bin/bash" > $launcher
# 	echo >> $launcher
# 	echo "if [ ! -e JanusC ]; then" >> $launcher
# 	echo "    echo \"Please, compile Janus using make in the main folder\"" >> $launcher
# 	echo "    return -1" >> $launcher
# 	echo "fi" >> $launcher
# 	echo "./JanusC" >> $launcher
# 	echo "stty sane" >> $launcher
# fi
# chmod +x $launcher

echo "*************************************************"
echo "***     Creating USB Rules for FERS board     ***"
echo "*************************************************"
# Create rule for USB privilege - the installer must be run as root
if [ $isroot -ne 0 ]; then     # from https://www.xmodulo.com/change-usb-device-permission-linux.html
	echo -e "${Yellow}Please, be aware that, as user, you might not have the permission to connect"
	echo -e "with FERS modules via USB. In case of USB connection issues, please re-run"
	echo -e "this installer as root with USB plugged in to create the permission rule for connecting via USB.${Clear}"
elif [ $isroot -eq 0 ]; then
	# Check idVendor and idProduct of WinUSB. It should be always the same
	usblist=`lsusb | grep 'Microchip Technology'`
	if [ "$usblist" != "" ]; then # USB connected
		bus=`echo $usblist | awk -F " " '{print $2}'`
		device=`echo $usblist | awk -F " " '{print $4}'`
		VV=`lsusb -v -s $bus:$device | grep idVendor`
		PP=`lsusb -v -s $bus:$device | grep idProduct`
		mVendor=`echo $VV | awk -F " " '{print $2}' | awk -F "x" '{print $2}'`
		mProduct=`echo $PP | awk -F " " '{print $2}' | awk -F "x" '{print $2}'`
	else # USB not connected
		mVendor=04d8
		mProduct=0053
		echo -e "${Yellow}Warning: the idVendor and idProduct are set as default value. Please check these values"
		echo -e "${Yellow}by plugging in the FERS via USB and typing on shell 'lsusb -v'${Clear}"
		echo
		echo
	fi

	FILERULE="/etc/udev/rules.d/50-myusb.rules"
	USBRULE='SUBSYSTEMS=="usb",ATTRS{idVendor}=="'$mVendor'",ATTRS{idProduct}=="'$mProduct'",GROUP="users",MODE="0666"'
	# USBRULE='SUBSYSTEMS=="usb",ATTRS{idVendor}=="04d8",ATTRS{idProduct}=="0053",GROUP="users",MODE="0666"'
	echo "Creating the permission rule file " $FILERULE " to let JanusC connect via USB without the need of being root ..."
	if [ ! -e $FILERULE ]; then
		echo "Creating the Rule File ... "
		sudo touch $FILERULE
	fi
	grep -Fxq $USBRULE $FILERULE
	res=$? 
	if [ $res -ne 0 ]; then
		echo $USBRULE >> $FILERULE
		echo "DONE"
	else
		echo "The permission rule is already present on this PC."
	fi
	
	echo "If the USB connection doesn't work, try to unplug and plug back the usb connector,"
	echo "reboot and execute 'sudo udevadm control --reload' and 'sudo udevadm trigger'"
fi

echo
echo "*************************************************"
echo "Installation completed"
#echo
#echo "N.B.: if you use JanusC from shell please launch it"
#echo "with the launcher script ./launch_JanusC.sh"
echo "*************************************************"


