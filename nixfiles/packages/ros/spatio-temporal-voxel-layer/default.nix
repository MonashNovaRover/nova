{ lib, buildRosPackage, fetchurl, ament-cmake, ament-lint-auto, builtin-interfaces, geometry-msgs, laser-geometry, message-filters, nav2-costmap-2d, openexr, openvdb, pcl, pcl-conversions, pluginlib, rclcpp, rosidl-default-generators, rosidl-default-runtime, sensor-msgs, std-msgs, std-srvs, tf2-geometry-msgs, tf2-ros, tf2-sensor-msgs, visualization-msgs }:
buildRosPackage {
  pname = "ros-jazzy-spatio-temporal-voxel-layer";
  version = "2.5.5";

  src = fetchurl {
    url = "https://github.com/SteveMacenski/spatio_temporal_voxel_layer-release/archive/release/jazzy/spatio_temporal_voxel_layer/2.5.5.tar.gz";
    name = "2.5.5.tar.gz";
    sha256 = "03l0x7g6rjk2y4fjcq02za0jzmhb3rnzj20a0lz69cmnxfxjg4ls";
  };

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake rosidl-default-generators openvdb ];
  checkInputs = [ ament-lint-auto ];
  propagatedBuildInputs = [ builtin-interfaces geometry-msgs laser-geometry message-filters nav2-costmap-2d openexr openvdb pcl pcl-conversions pluginlib rclcpp rosidl-default-runtime sensor-msgs std-msgs std-srvs tf2-geometry-msgs tf2-ros tf2-sensor-msgs visualization-msgs ];
  nativeBuildInputs = [ ament-cmake ];

  meta = {
    description = "The spatio-temporal 3D obstacle costmap package";
    license = with lib.licenses; [ "LGPL-2.1-only" ];
  };

  patchPhase = ''
  echo $CMAKE_PREFIX_PATH
  # Replace openvdb_vendor with openvdb
  substituteInPlace CMakeLists.txt \
    --replace "find_package(openvdb_vendor REQUIRED)" "find_package(OpenVDB REQUIRED)" \
    --replace "openvdb_vendor" ""

  substituteInPlace package.xml \
    --replace "<depend>openvdb_vendor</depend>" "<depend>openvdb</depend>"
  '';

  cmakeFlags = [
    "-DCMAKE_PREFIX_PATH=${openvdb}/lib/cmake/openvdb"
  ];
  shellHook = ''
    export CMAKE_MODULE_PATH="${openvdb}/lib/cmake/OpenVDB:$CMAKE_MODULE_PATH"
    export CMAKE_PREFIX_PATH="${openvdb}/lib/cmake/openvdb:$CMAKE_PREFIX_PATH"
  '';

}