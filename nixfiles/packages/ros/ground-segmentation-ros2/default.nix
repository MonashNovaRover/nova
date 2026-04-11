{ 
  buildRosPackage,
  fetchgit,
  ament-cmake,
  rclcpp,
  sensor-msgs,
  tf2-ros,
  tf2-geometry-msgs,
  pcl-conversions,
  pcl-ros,
  ground-segmentation,
  std-srvs,
  nanoflann
}:

buildRosPackage {
  pname = "ground-segmentation-ros2";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/dfki-ric/ground_segmentation_ros2";
    rev = "b9aace4fa3ea75845d5dfc308321c8419a202ca4"; 
    hash = "sha256-bLtYc9aX0uoLQQDg5G7jPHv0Xhe1JPd/V1HG+LwfXu0="; 
  };

  buildType = "ament_cmake"; 

  nativeBuildInputs = [ 
    ament-cmake
  ];

  buildInputs = [
    rclcpp
    sensor-msgs
    tf2-ros
    tf2-geometry-msgs
    pcl-conversions
    pcl-ros
    ground-segmentation
    std-srvs
    nanoflann
  ];
}