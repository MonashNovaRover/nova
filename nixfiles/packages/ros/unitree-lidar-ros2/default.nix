{ buildRosPackage, 
  fetchurl, 
  ament-cmake, 
  rclcpp, 
  std-msgs, 
  geometry-msgs, 
  pcl-conversions, 
  pcl, 
  tf2-ros, 
  sensor-msgs, 
  libpcap, 
  libusb1, 
}:

buildRosPackage rec {
  pname = "unitree-lidar-ros2";
  version = "2.0.10";

  src = fetchurl {
    url = "https://github.com/unitreerobotics/unilidar_sdk2/archive/refs/tags/v${version}.tar.gz";
    hash = "sha256-Ld+5XqSas+1l1JCTptbtF0ZCsuYJSCIfUbmiG7MU2Qg=";
  };

  sourceRoot = "unilidar_sdk2-${version}/unitree_lidar_ros2/src/unitree_lidar_ros2";

  buildType = "ament_cmake";
  
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [ 
    ament-cmake 
    rclcpp
    std-msgs
    geometry-msgs
    pcl-conversions
    pcl
    tf2-ros
    sensor-msgs
    libpcap
    libusb1
  ];
}