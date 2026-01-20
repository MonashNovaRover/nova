{ 
  buildRosPackage,
  callPackage,
  fetchgit,
  ament-cmake,
  rclcpp,
  vikit-common,
  visualization-msgs,
  tf2,
  tf2-ros,
  opencv,
  eigen,
  sophus,
  tf2-geometry-msgs,
  rosidl-default-generators,
  breakpointHook,
}:

buildRosPackage {
  pname = "vikit-ros";
  version = "3.5";

  src = fetchgit {
    url = "https://github.com/Robotic-Developer-Road/rpg_vikit";
    rev = "4b7abc838f5d2ca9137f70f122eaaeff9eaf0f50";
    sparseCheckout = [
      "vikit_ros"
    ];
    hash = "sha256-9+PLaSKqnVC9+WNGXPGMmIelAbG3ry6z0k2zzaEaLnI=";
  };


  sourceRoot = "rpg_vikit-4b7abc8/vikit_ros";
  
  buildType = "ament_cmake";
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [
    rclcpp
    vikit-common
    visualization-msgs
    tf2
    tf2-ros
    opencv
    eigen
    sophus
    tf2-geometry-msgs
    rosidl-default-generators
    breakpointHook
  ];
}