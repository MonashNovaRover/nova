#!/bin/sh

# USB Isoc Bandwidth Printer
#
# Prints the allocated Isoc bandwidth for all usb devices, sorted by bus.
# These values are per usb microframe. for usb 2 you have at most about 
# 5*1024 B/uframe per usb bus (but what software sees as one bus may be
# several in hardware).
#
# FIXME: we don't consider if there are alternate usb configurations.
#
# Author: Orlando Chamberlain

for bus in $(ls /sys/bus/usb/devices | grep usb); do
	busnum=$(echo $bus | sed -e s/usb//g)
	cd /sys/bus/usb/devices/
	echo $bus speed: $(cat $bus/speed)M

	for device in $(ls | grep ${busnum}- | grep -v :); do
	    cd -P /sys/bus/usb/devices/$device

	    busnum=$(cat busnum)
	    devnum=$(cat devnum)
	    for interface in $(ls | grep :); do
	    	cd $interface

			for ep in $(ls | grep "^ep_"); do
				altsetting=$(cat bAlternateSetting)
				cd $ep
				if [ $(cat type) = "Isoc" ]; then
					addr=$(cat bEndpointAddress)

                    lsusb -s $busnum:$devnum
					# this is terrible! but the wMaxPacketSize in sysfs 
					# is not the same as it drops the 3x/2x/1x information
					# so I have to do it this way.
					lsusb -s $busnum:$devnum -v 2>/dev/null | grep \
						-e "^      Endpoint Descriptor" \
						-e wMaxPacketSize \
						-e bAlternateSetting \
				        -e "^    Interface Descriptor:" \
				        -e bInterfaceNumber \
				        -e "Transfer Type" \
				        -e bEndpointAddress \
				        | grep "bEndpointAddress     0x$addr" -B4 -A2 \
				        | grep "bAlternateSetting.* $altsetting" -B2 -A4 \
				        | grep -e wMaxPacketSize -e bAlternateSetting
				fi
				cd ..
			done
			cd ..
		done
	done
done
