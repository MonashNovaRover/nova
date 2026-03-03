{ lib
, buildRosPackage
, ament-cmake
, pkg-config
, rclcpp
, generate-parameter-library
, std-msgs
, std-srvs
, systemd
, nova-camera-msgs
, gst_all_1
, libnice
, v4l-utils
, gst-bridge
}:

buildRosPackage {
  name = "nova-cameras3";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cameras3-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake pkg-config ];

  buildInputs = [
    rclcpp
    std-msgs
    std-srvs
    generate-parameter-library
    systemd
    nova-camera-msgs
    
    gst_all_1.gstreamer        # base
    gst_all_1.gstreamermm      # cpp api
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-good
    gst_all_1.gst-plugins-bad
    gst_all_1.gst-plugins-ugly
    gst_all_1.gst-libav
    gst_all_1.gst-vaapi
    gst_all_1.gst-plugins-rs  # webrtc
    libnice                   # needed for webrtc
    v4l-utils                 # v4l-ctl
    gst-bridge                # ros-gst-bridge/rosimagesrc
  ];

  postInstall = ''
    mkdir $out/bin
    if [ -d "${gst_all_1.gst-plugins-rs}/bin" ]; then
      for file in ${gst_all_1.gst-plugins-rs}/bin/*; do
        ln -sf "$file" "$out/bin/"
      done
    fi
  '';

}
