{ 
  buildRosPackage,
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
}:

buildRosPackage {
  pname = "vikit-ros";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/Robotic-Developer-Road/rpg_vikit";
    rev = "4b7abc838f5d2ca9137f70f122eaaeff9eaf0f50";
    sparseCheckout = [
      "vikit_ros"
    ];
    hash = "sha256-9+PLaSKqnVC9+WNGXPGMmIelAbG3ry6z0k2zzaEaLnI=";
  };

  patches = [
    ./patches/timeout.patch
  ];


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
  ];

  postPatch = ''
    sed -i '/{CMAKE_SYSTEM_PROCESSOR}")/aSET(CMAKE_PREFIX_PATH ''${CMAKE_PREFIX_PATH} "${vikit-common}/share/vikit_common/CMakeModules/build/")' CMakeLists.txt
  '';
}