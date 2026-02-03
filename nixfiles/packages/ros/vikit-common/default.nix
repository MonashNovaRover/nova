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
  breakpointHook,
}:

stdenv.mkDerivation rec {
  name = "vikit-common";
  version = "0.0.0";

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
    breakpointHook
  ];

  buildInputs = [
    opencv
    sophus
    boost
    fmt
    tbb_2021_11
    pkg-config
  ];
  
  postPatch = ''
    sed -i '/lib )/aINSTALL(DIRECTORY ''${CMAKE_CURRENT_BINARY_DIR} DESTINATION ''${CMAKE_INSTALL_PREFIX}/share/''${PROJECT_NAME}/CMakeModules )' CMakeLists.txt
  '';
}
