{ lib, 
  buildRosPackage, 
  ament-cmake, 
  rclcpp, 
  nav2-behavior-tree, 
  pluginlib, 
  rosidl-default-generators, 
  behaviortree-cpp, 
  geometry-msgs, 
  vision-msgs, 
  aruco-opencv-msgs, 
  visualization-msgs, 
  tf2-ros, 
  nav2-util, 
  std-srvs, 
  launch, 
  launch-ros, 
}:

buildRosPackage {
  name = "behavior-tree";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-behavior-tree-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ 
    pluginlib 
    std-srvs 
    rclcpp 
    nav2-behavior-tree 
    behaviortree-cpp 
    nav2-util 
    tf2-ros 
    geometry-msgs 
    vision-msgs 
    aruco-opencv-msgs 
    visualization-msgs 
  ];
  propagatedBuildInputs = [ launch launch-ros ];
}
