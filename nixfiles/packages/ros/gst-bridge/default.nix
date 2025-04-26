{ buildRosPackage, 
  fetchgit, 
  ament-cmake, 
  ament-cmake-copyright, 
  ament-cmake-cppcheck, 
  ament-cmake-uncrustify, 
  ament-lint-auto, 
  ament-lint-common, 
  pkg-config, 
  rclcpp, 
  rclcpp-components, 
  gst_all_1, 
  std-msgs, 
  audio-msgs, 
  sensor-msgs, 
  builtin-interfaces, 
  pcre2, # Package 'libpcre2-8', required by 'glib-2.0', not found
  libunwind, # Package 'libunwind', required by 'gstreamer-1.0', not found
  util-linux, # Package 'mount', required by 'gio-2.0', not found
  elfutils, # Package 'libdw', required by 'gstreamer-1.0', not found
  libselinux, # Package 'libselinux', required by 'gio-2.0', not found
  zstd, # Package 'libzstd', required by 'libelf', not found
  libsepol, # ackage 'libsepol', required by 'libselinux', not found
  orc, # Package 'orc-0.4', required by 'gstreamer-audio-1.0', not found
  breakpointHook, 
}:

buildRosPackage {
  pname = "ros-jazzy-gst-bridge";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/BrettRD/ros-gst-bridge";
    sparseCheckout = [
      "gst_bridge"
    ];
    hash = "sha256-4k3BMltuFaoJKrxlv/LFEwoq5Lazau0c/yS4CzBsitM=";
  };

  sourceRoot = "ros-gst-bridge/gst_bridge";

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake pkg-config breakpointHook ];
  checkInputs = [ ament-cmake-copyright ament-cmake-cppcheck ament-cmake-uncrustify ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rclcpp rclcpp-components gst_all_1.gstreamer gst_all_1.gst-plugins-base std-msgs audio-msgs sensor-msgs builtin-interfaces pcre2 libunwind util-linux elfutils libselinux zstd libsepol orc ];
  nativeBuildInputs = [ ament-cmake ];
}