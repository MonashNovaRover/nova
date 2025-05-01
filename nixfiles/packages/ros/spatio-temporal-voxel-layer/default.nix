{ lib, 
 buildRosPackage, 
 fetchFromGitHub, 
 ament-cmake, 
 ament-cmake-copyright, 
 ament-cmake-cppcheck, 
 ament-cmake-uncrustify, 
 ament-lint-auto, 
 ament-lint-common, 
 pkg-config, 
 nav2-costmap-2d, 
 geometry-msgs, 
 pluginlib, 
 sensor-msgs, 
 std-msgs, 
 laser-geometry, 
 message-filters, 
 pcl-conversions, 
 rclcpp, 
 tf2-ros, 
 tf2-geometry-msgs, 
 tf2-sensor-msgs, 
 visualization-msgs, 
 builtin-interfaces, 
 rosidl-default-generators, 
 std-srvs, 
 openvdb, 
 boost, 
 pcl, 
 c-blosc, 
 openexr, 
 rosidl-default-runtime, 
 tbb_2021_11, 
 zlib, 
 breakpointHook, 
}:

buildRosPackage rec {
  pname = "ros-jazzy-spatio-temporal-voxel-layer";
  version = "2.5.5";

  src = fetchFromGitHub {
    owner = "SteveMacenski";
    repo = "spatio_temporal_voxel_layer";
    rev = "2.5.5";
    hash = "sha256-Qk2k6aa+WDgXwz98l0MwB1LLb8yBULaue1u0mh6vVHc=";
  };

  sourceRoot = "${src.name}/spatio_temporal_voxel_layer";

  buildType = "ament_cmake";
  
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ 
    nav2-costmap-2d 
    geometry-msgs 
    pluginlib 
    sensor-msgs 
    std-msgs 
    laser-geometry 
    message-filters 
    pcl-conversions 
    rclcpp 
    tf2-ros 
    tf2-geometry-msgs 
    tf2-sensor-msgs 
    visualization-msgs 
    builtin-interfaces 
    rosidl-default-generators 
    std-srvs 
    openvdb 
    boost 
    pcl 
    c-blosc 
    openexr 
    rosidl-default-runtime 
    tbb_2021_11 
    zlib 
  ];
  nativeBuildInputs = [ ament-cmake pkg-config ];

  meta = {
    description = "The spatio-temporal 3D obstacle costmap package";
    license = with lib.licenses; [ "LGPL-2.1-only" ];
  };

  postPatch = ''
    substituteInPlace CMakeLists.txt\
      --replace "find_package(openvdb_vendor REQUIRED)" "find_package(OpenVDB REQUIRED)"\
      --replace "OpenVDB:" "$"\
      --replace ":openvdb" "{OpenVDB_LIBRARIES}"\
      --replace "openvdb_vendor" ""
    sed -i '/set(CMAKE_MODULE_PATH/alist(APPEND CMAKE_MODULE_PATH "${openvdb.dev}/lib/cmake/OpenVDB/")' CMakeLists.txt
    sed -i '/find_package(OPENVDB REQUIRED)/afind_package(TBB REQUIRED)' CMakeLists.txt
    sed -i '/{OpenVDB_LIBRARIES}/aTBB::tbb' CMakeLists.txt
  '';
}