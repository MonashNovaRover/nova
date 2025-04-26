{ lib, buildRosPackage, fetchFromGitHub, ament-cmake, ament-cmake-vendor-package, ament-lint-auto, boost, builtin-interfaces, c-blosc, git, geometry-msgs, laser-geometry, message-filters, nav2-costmap-2d, openexr, openvdb, pcl, pcl-conversions, pluginlib, rclcpp, rosidl-default-generators, rosidl-default-runtime, sensor-msgs, std-msgs, std-srvs, tbb_2021_11, tf2-geometry-msgs, tf2-ros, tf2-sensor-msgs, visualization-msgs, zlib }:
let
githubrepo = fetchFromGitHub {
  owner = "SteveMacenski";
  repo = "spatio_temporal_voxel_layer";
  rev = "2.5.5";
  hash = "sha256-Qk2k6aa+WDgXwz98l0MwB1LLb8yBULaue1u0mh6vVHc=";
};
openvdb-vendor = buildRosPackage rec {
  pname = "ros-jazzy-openvdb-vendor";
  version = "2.5.5";

  src = githubrepo;

  sourceRoot = "${src.name}/openvdb_vendor";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake ament-cmake-vendor-package git ];
  propagatedBuildInputs = [ boost c-blosc openvdb tbb_2021_11 zlib ];
  nativeBuildInputs = [ ament-cmake ament-cmake-vendor-package git ];

  meta = {
    description = "Wrapper around OpenVDB, if not found on the system, will compile from source";
    license = with lib.licenses; [ "LGPL-2.1-only" "MPL-2.0-license" ];
    position = "./default.nix";
  };

  file = "CMakeLists.txt";

  vendor = lib.tarSource {} (
    fetchFromGitHub rec {
      owner = "AcademySoftwareFoundation";
      repo = "openvdb";
      rev = "v10.0.1";
      hash = "sha256-kaf5gpGYVWinmnRwR/IafE1SJcwmP2psfe/UZdtH1Og=";
      name = rev;
    });

  postPatch = ''
    substituteInPlace CMakeLists.txt\
      --replace "https://github.com/AcademySoftwareFoundation/openvdb.git" "file://${vendor}"\
      --replace "VCS_TYPE git" "VCS_TYPE tar"
  '';
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
  propagatedBuildInputs = [ builtin-interfaces geometry-msgs laser-geometry message-filters nav2-costmap-2d openexr openvdb-vendor pcl pcl-conversions pluginlib rclcpp rosidl-default-runtime sensor-msgs std-msgs std-srvs tf2-geometry-msgs tf2-ros tf2-sensor-msgs visualization-msgs ];
  nativeBuildInputs = [ ament-cmake ];

  meta = {
    description = "The spatio-temporal 3D obstacle costmap package";
    license = with lib.licenses; [ "LGPL-2.1-only" ];
  };
}