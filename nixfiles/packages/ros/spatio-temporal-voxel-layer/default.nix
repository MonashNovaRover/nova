{ lib, 
 buildRosPackage, 
 fetchFromGitHub, 
 ament-cmake, 
 pkg-config, 
 ament-lint-auto, 
 boost, 
 builtin-interfaces, 
 c-blosc, 
 geometry-msgs, 
 laser-geometry, 
 message-filters,
 nav2-costmap-2d,
 openexr, 
 openvdb, 
 pcl, 
 pcl-conversions, 
 pluginlib, 
 rclcpp, 
 rosidl-default-generators, 
 rosidl-default-runtime, 
 sensor-msgs, 
 std-msgs, 
 std-srvs, 
 tbb_2021_11, 
 tf2-geometry-msgs, 
 tf2-ros, 
 tf2-sensor-msgs, 
 visualization-msgs, 
 zlib 
}:

let
githubrepo = fetchFromGitHub {
  owner = "SteveMacenski";
  repo = "spatio_temporal_voxel_layer";
  rev = "2.5.5";
  hash = "sha256-Qk2k6aa+WDgXwz98l0MwB1LLb8yBULaue1u0mh6vVHc=";
};
in
buildRosPackage rec {
  pname = "ros-jazzy-spatio-temporal-voxel-layer";
  version = "2.5.5";

  src = githubrepo;

  sourceRoot = "${src.name}/spatio_temporal_voxel_layer";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake rosidl-default-generators ];
  checkInputs = [ ament-lint-auto ];
  propagatedBuildInputs = [ tbb_2021_11 zlib boost builtin-interfaces c-blosc geometry-msgs laser-geometry message-filters nav2-costmap-2d openexr openvdb pcl pcl-conversions pluginlib rclcpp rosidl-default-runtime sensor-msgs std-msgs std-srvs tf2-geometry-msgs tf2-ros tf2-sensor-msgs visualization-msgs ];
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
      --replace 'PROJECT_SOURCE_DIR}/cmake/")' 'PROJECT_SOURCE_DIR}/cmake/" )'\
      --replace "openvdb_vendor" ""
    sed -i '/set(CMAKE_MODULE_PATH/alist(APPEND CMAKE_MODULE_PATH "${openvdb.dev}/lib/cmake/OpenVDB/")' CMakeLists.txt
    sed -i '/find_package(OPENVDB REQUIRED)/afind_package(TBB REQUIRED)' CMakeLists.txt
    sed -i '/{OpenVDB_LIBRARIES}/aTBB::tbb' CMakeLists.txt
    cat CMakeLists.txt
  '';
}