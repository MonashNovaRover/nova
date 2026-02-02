{ 
  buildRosPackage,
  fetchgit,
  ament-cmake,
  mimalloc,
  rclcpp,
  rclpy,
  geometry-msgs,
  nav-msgs,
  sensor-msgs,
  std-msgs,
  visualization-msgs,
  pcl-ros,
  pcl-conversions,
  tf2,
  tf2-ros,
  livox-ros-driver2,
  vikit-common,
  vikit-ros,
  cv-bridge,
  image-transport,
  image-geometry,
  eigen,
  pcl,
  opencv,
  opencv4,
  sophus,
  boost,
  fmt,
  libusb1,
  libpcap,
  breakpointHook,
  rosbag2-cpp,
  rosbag2-storage,
  message-filters
}:

buildRosPackage {
  pname = "fast-calib";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/Robotic-Developer-Road/FAST-Calib";
    rev = "ab39b9c89995e144b49fad982add1b1e17060ff3";
    hash = "sha256-5KJ5uen1rTej2t6m8rXWDTMiB43nzcPKm67XpcYutpg=";
  };

  patches = [
    ./patches/api_changes.patch
  ];

  buildType = "ament_cmake";
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [
    rclcpp
    geometry-msgs
    nav-msgs
    sensor-msgs
    std-msgs
    cv-bridge
    image-transport
    image-geometry
    pcl-ros
    pcl-conversions
    tf2-ros
    livox-ros-driver2
    eigen
    pcl
    opencv4
    rosbag2-cpp
    rosbag2-storage
    cv-bridge
  ];

  #postPatch = ''
  #  sed -i '/g")/aSET(CMAKE_PREFIX_PATH ''${CMAKE_PREFIX_PATH} "${vikit-common}/share/vikit_common/CMakeModules/build/")' CMakeLists.txt
  #  sed -i CMakeLists.txt \
  #    -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_common@${vikit-common}@g' \
  #    -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_ros@${vikit-ros}@g'
  #'';
}
