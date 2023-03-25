
storage=$1
bag_file=$2

topics="/depth_camera/d435_1/image /depth_camera/d435_1/cloud /T265/pose"
remaps="/depth_camera/d435_1/image:=/depth_camera/d435_1/image/old /depth_camera/d435_1/cloud:=/depth_camera/d435_1/cloud/old /T265/pose:=/T265/pose/old"


ros2 bag play -l -s $storage -r 0.1 $bag_file --topics $topics -m $remaps
