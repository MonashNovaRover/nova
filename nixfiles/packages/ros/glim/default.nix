{
  buildRosPackage,
  fetchgit,
  cmake,
  ament-cmake,
  fmt,
  spdlog,
  boost,
  eigen,
  opencv,
  gtsam,
  gtsam-points,
  llvmPackages
}:

buildRosPackage {
  pname = "glim";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/koide3/glim";
    rev = "2262aafa2369acff6afab2268a7743aec7a7fa49";
    hash = "sha256-vfEVvks9ygfchsqFMdnCVi/n5StZLGoUsFGbfUd9OZw=";
  };

  postPatch = ''
    export ROS_VERSION=2
    export ROS_DISTRO=jazzy
  '';

  buildType = "cmake";

  cmakeFlags = [
    "-DBUILD_WITH_CUDA=OFF"
    "-DBUILD_WITH_VIEWER=OFF"
  ];

  nativeBuildInputs = [
    cmake
    ament-cmake
  ];

  buildInputs = [
    fmt
    spdlog
    boost
    eigen
    opencv
    gtsam
    gtsam-points
    llvmPackages.openmp
  ];
}
