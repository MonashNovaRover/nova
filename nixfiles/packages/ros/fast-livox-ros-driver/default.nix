{ 
  buildRosPackage, 
  fetchgit, 
  ament-cmake, 
  ament-cmake-auto, 
  rcl-interfaces,
  rclcpp-components,
  rclcpp, 
  sensor-msgs,
  livox-sdk2
}:

buildRosPackage {
  pname = "fast-livox-ros-driver";
  version = "0.0.0";

  src = fetchgit {
    name = "fast-livox-ros-driver-source";
    url = "https://github.com/zz990099/fast_livox_ros_driver";
    rev= "db654af7acc8e033d2bd06336d950cb592049ac7";
    hash = "sha256-S195RXCI2lT0L4P/MZL0cWrOtVGNUdAjMEgk26VTOac=";
  };

  buildType = "ament_cmake";
  
  nativeBuildInputs = [ 
    ament-cmake 
    ament-cmake-auto
  ];
  
  propagatedBuildInputs = [ 
    rclcpp 
    rcl-interfaces
    rclcpp-components
    sensor-msgs
    livox-sdk2
  ];

}