{ 
  buildRosPackage,
  fetchgit,
  ament-cmake-auto,
  ament-cmake-python,
  pcl,
  ceres-solver,
  gtsam,
  opencv,
  boost,
  iridescence,
  llvmPackages,
  rclcpp,
  rosbag2,
  cv-bridge,
  glfw,
  assimp,
}:

buildRosPackage {
  pname = "direct-visual-lidar-calibration";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/koide3/direct_visual_lidar_calibration";
    rev = "02a0dc039f5509708f384be4ff3228e0ae09352d";
    hash = "sha256-F2F761iDMIJSzeSKQoKLGdQcxdp00TZlDu2srBJgGt4=";
  };

  postPatch = ''
    export ROS_VERSION=2
    export ROS_DISTRO="jazzy"
  '';

  buildType = "ament_cmake";

  nativeBuildInputs = [ 
    ament-cmake-auto 
    ament-cmake-python
  ];

  buildInputs = [
    pcl
    ceres-solver
    gtsam
    opencv
    boost
    iridescence
    llvmPackages.openmp
    rclcpp
    rosbag2
    cv-bridge
    glfw
    assimp
  ];
}