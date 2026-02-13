{ 
  buildRosPackage, 
  callPackage, 
  fetchFromGitHub,
  fetchgit, 
  ament-cmake, 
  boost,
  livox-ros-driver2,
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
  visualization-msgs,
  nav-msgs,
  tf2,
  tf2-ros,
  cv-bridge,
  opencv,
  openblas,
  yaml-cpp
}:

buildRosPackage rec {
  pname = "r3live";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/Yajiang/r3live";
    rev = "36746f26d8c95cd466811f2f72d05c0cd0d129eb";
    hash = "sha256-V4aSXIr12joTB15SaRUHAEXaxqr72x1BCBO/9NdTtjg=";
    sparseCheckout = [
      "r3live"
    ];
  };

  sourceRoot = "${src.name}/r3live";

  buildType = "ament_cmake";
  
  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    ament-cmake
    rclcpp
    boost
    livox-ros-driver2
    ament-cmake-auto
    visualization-msgs
    nav-msgs
    tf2
    tf2-ros
    cv-bridge
    pcl-conversions
    yaml-cpp
  ];

  patches = [
    ./patches/cv_upgrade.patch
  ];
}