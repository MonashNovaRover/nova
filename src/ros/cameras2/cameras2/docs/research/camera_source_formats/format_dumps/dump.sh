#!/bin/sh

if which v4l2-ctl 2>/dev/null; then
  true
else
  echo "run me in 'nix-shell -p v4l-utils'"
  exit 1
fi

for dir in $(echo /sys/bus/usb/drivers/uvcvideo/*/driver); do
  dir=$(echo $dir | rev | cut -d / -f 2- | rev)
  dir=$(readlink -f $dir)/..
  name="$(cat $dir/idVendor)_$(cat $dir/idProduct)_$(cat $dir/manufacturer)_$(cat $dir/product)"
  truncate -s 0 "$name.txt"
  for node in $dir/*/video4linux/video*; do
    echo $dir ha $node
    node=$(basename $node)
    v4l2-ctl --list-formats-ext -d /dev/$node >> "$name.txt"
  done

  lsusb -s $(cat $dir/busnum):$(cat $dir/devnum) -v > "$name.lsusb.txt"

done



