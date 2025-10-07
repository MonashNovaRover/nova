{ 
  buildRosPackage, 
  callPackage, 
  fetchgit, 
  ament-cmake, 
  ament-cmake-auto, 
  ament-cmake-copyright, 
  ament-cmake-cppcheck, 
  ament-cmake-uncrustify, 
  ament-lint-auto, 
  ament-lint-common, 
  rclcpp, 
  rclcpp-components, 
  pluginlib, 
  std-srvs, 
  std-msgs, 
  builtin-interfaces, 
  rosidl-default-generators, 
  pcl, 
  eigen, 
  flann, 
  pcl-conversions, 
  livox-sdk2, 
}:

buildRosPackage {
  pname = "livox-ros-driver2";
  version = "0.0.0";

  src = fetchgit {
    name = "livox-ros-driver2-source";
    url = "https://github.com/Livox-SDK/livox_ros_driver2";
    rev= "6b9356cadf77084619ba406e6a0eb41163b08039";
    hash = "sha256-H2HBuTDkj5kcoANZU/MKZDt94a9oUd4KO73IBPOXBeU=";
  };

  patchPhase = ''
    # mv -f launch_ROS2 launch
    # mv -f package_ROS2.xml package.xml

    # They do this:
    cp -f package_ROS2.xml package.xml
    cp -rf launch_ROS2/ launch/
  '';

  buildType = "ament_cmake";
  
  checkInputs = [ 
    ament-cmake-copyright 
    ament-cmake-cppcheck 
    ament-cmake-uncrustify 
    ament-lint-auto 
    ament-lint-common 
  ];
  
  nativeBuildInputs = [ 
    ament-cmake 
    ament-cmake-auto
  ];

  buildInputs = [ 
    ament-cmake 
    std-msgs
    builtin-interfaces
    rosidl-default-generators
    pcl
    livox-sdk2
    eigen
    flann
    pcl-conversions
  ];
  
  propagatedBuildInputs = [ 
    rclcpp 
    rclcpp-components 
    pluginlib 
    std-srvs 
    std-msgs
    builtin-interfaces
  ];

  cmakeFlags = [
    "-DHUMBLE_ROS=humble"
  ];
}