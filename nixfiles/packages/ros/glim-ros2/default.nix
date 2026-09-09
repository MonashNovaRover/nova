{
  buildRosPackage,
  fetchgit,
  cmake,
  ament-cmake,
  ament-cmake-auto,
  boost,
  eigen,
  gtsam,
  gtsam-points,
  glim,
  rclcpp,
  rclcpp-components,
  ament-index-cpp,
  rosbag2-cpp,
  rosbag2-compression,
  rosbag2-storage,
  cv-bridge,
  image-transport,
  nav-msgs,
  sensor-msgs,
  geometry-msgs,
  tf2-ros
}:

buildRosPackage {
  pname = "glim-ros2";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/koide3/glim_ros2";
    rev = "4d4ec524ccf1b02aa09b0af2af767ecc54343798";
    hash = "sha256-ZBKHg+hOTSnRSs2VZucEqo1XBPVmU8KfBMsmb60aM6M=";
  };

  postPatch = ''
    export ROS_VERSION=2
    export ROS_DISTRO=jazzy
  '';

  buildType = "cmake";

  cmakeFlags = [
    "-DBUILD_WITH_CUDA=OFF"
    "-DBUILD_WITH_VIEWER=ON"
  ];

  nativeBuildInputs = [
    cmake
    ament-cmake
    ament-cmake-auto
  ];

  buildInputs = [
    boost
    eigen
    gtsam
    gtsam-points
    glim
    rclcpp
    rclcpp-components
    ament-index-cpp
    rosbag2-cpp
    rosbag2-compression
    rosbag2-storage
    cv-bridge
    image-transport
    nav-msgs
    sensor-msgs
    geometry-msgs
    tf2-ros
  ];
}
