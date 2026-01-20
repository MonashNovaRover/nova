{
  stdenv,
  fetchgit,
  ament-cmake,
  opencv,
  sophus,
  boost,
  fmt,
  tbb_2021_11,
  pkg-config,
}:

stdenv.mkDerivation rec {
  name = "vikit-common";
  version = "3.0";

  src = fetchgit {
    url = "https://github.com/Robotic-Developer-Road/rpg_vikit";
    rev = "4b7abc838f5d2ca9137f70f122eaaeff9eaf0f50";
    sparseCheckout = [
      "vikit_common"
    ];
    hash = "sha256-egPWl/qiqmkg/LP7hxnZ5P2/OonSBALYQqbdpUplpi8=";
  };

  sourceRoot = "rpg_vikit-4b7abc8/vikit_common";
  
  buildType = "ament_cmake";
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [
    opencv
    sophus
    boost
    fmt
    tbb_2021_11
    pkg-config
  ];
}
