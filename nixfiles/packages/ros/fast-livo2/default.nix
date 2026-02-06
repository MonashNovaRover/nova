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
  visualization-msgs,
  pcl-ros,
  pcl-conversions,
  tf2-ros,
  livox-ros-driver2,
  vikit-common,
  vikit-ros,
  cv-bridge,
  image-transport,
  eigen,
  pcl,
  opencv,
  sophus,
  boost,
  fmt,
  libusb1,
  libpcap,
  breakpointHook,
}:

buildRosPackage {
  pname = "fast-livo2";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/Robotic-Developer-Road/FAST-LIVO2";
    rev = "837b7bbc1431cb04cf936528e52c83c835efba8e";
    hash = "sha256-Dqqs2njlVrUVbzYeUgGp07F0335ntSJPkAvgDhUcH4A=";
  };

  patches = [
    ./patches/memory_bug.patch
    ./patches/pc2_for_livox.patch
    ./patches/pcd_save_dir.patch
  ];

  buildType = "ament_cmake";
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [
    mimalloc
    rclcpp
    rclpy
    geometry-msgs
    nav-msgs
    sensor-msgs
    visualization-msgs
    pcl-ros
    pcl-conversions
    tf2-ros
    livox-ros-driver2
    vikit-common
    vikit-ros
    cv-bridge
    image-transport
    eigen
    pcl
    opencv
    sophus
    boost
    fmt
    libusb1
    libpcap
    breakpointHook
  ];

  postPatch = ''
    # Change ROOT_DIR from FAST-LIVO2 to /home/nova.
    # Needed for saving .pcd files from FAST-LIVO2.
    export USER_HOME_DIR=${builtins.getEnv "HOME"}
    sed -i CMakeLists.txt \
      -e 's@''${CMAKE_CURRENT_SOURCE_DIR}@$ENV{USER_HOME_DIR}@g'

    # Point FAST-LIVO2 cmake to vikit_common/CMakeModules. 
    # Needed for building FAST-LIVO2.
    sed -i '/g")/aSET(CMAKE_PREFIX_PATH ''${CMAKE_PREFIX_PATH} "${vikit-common}/share/vikit_common/CMakeModules/build/")' CMakeLists.txt

    # Point FAST-LIVO2 cmake to vikit_common/install. 
    # Needed for running FAST-LIVO2.
    sed -i CMakeLists.txt \
      -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_common@${vikit-common}@g' \
      -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_ros@${vikit-ros}@g'
  '';
}