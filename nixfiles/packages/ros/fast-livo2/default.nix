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
    sed -i '/g")/aSET(CMAKE_PREFIX_PATH ''${CMAKE_PREFIX_PATH} "${vikit-common}/share/vikit_common/CMakeModules/build/")' CMakeLists.txt
    sed -i CMakeLists.txt \
      -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_common@${vikit-common}@g' \
      -e 's@''${CMAKE_SOURCE_DIR}/../../install/vikit_ros@${vikit-ros}@g'
  '';
}