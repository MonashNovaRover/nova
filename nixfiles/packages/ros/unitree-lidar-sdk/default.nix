{ stdenv, 
  fetchurl, 
  cmake, 
  breakpointHook,
}:

stdenv.mkDerivation rec {
  pname = "unitree-lidar-sdk";
  version = "2.0.10";

  src = fetchurl {
    url = "https://github.com/unitreerobotics/unilidar_sdk2/archive/refs/tags/v${version}.tar.gz";
    hash = "sha256-Ld+5XqSas+1l1JCTptbtF0ZCsuYJSCIfUbmiG7MU2Qg=";
  };

  sourceRoot = "unilidar_sdk2-${version}/unitree_lidar_sdk";

  nativeBuildInputs = [ cmake breakpointHook ];

  # For context, the executable targets are never installed, but Nix requires packages to install at least one executable target, and will fail otherwise.
  # This patch fixes that by installing all executable targets (note one would be sufficient).
  postPatch = ''
    sed -i '/target_link_libraries(set_to_udp_mode  libunilidar_sdk2.a )/a\
      \ninstall(TARGETS example_lidar_udp example_lidar_serial set_ip_address set_to_serial_mode set_to_udp_mode\
      \n  DESTINATION lib/''${PROJECT_NAME}\
      \n)' CMakeLists.txt
  ''; 
}