echo "*************************************************"
echo "***     Creating USB Rules for FERS board     ***"
echo "*************************************************"

isroot=`id -u`

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