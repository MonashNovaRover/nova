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
    url = "https://github.com/MonashNovaRover/FAST-LIVO2";
    rev = "3be14ccf5cb01719a26ac7f4ff3a8f3d73184a66";
    hash = "sha256-imUKD7Ofo701Y3QibjzE8k+iHm/g6lzZVSYTsjCN51g=";
  };

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
    # Change ROOT_DIR from FAST-LIVO2 to /home/user/.ros
    # Needed for saving .pcd files from FAST-LIVO2.
    # TODO: patch it to get the $HOME env var at runtime
    export USER_HOME_DIR=/home/nova
    sed -i CMakeLists.txt \
      -e 's@''${CMAKE_CURRENT_SOURCE_DIR}@$ENV{USER_HOME_DIR}/.ros@g'

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