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
, wrapGAppsNoGuiHook
}:

buildRosPackage {
  name = "nova-cameras3";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cameras3-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake pkg-config gst_all_1.gstreamer wrapGAppsNoGuiHook ];

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
    v4l-utils                 # v4l2-ctl
    gst-bridge                # ros-gst-bridge/rosimagesrc
  ];

  dontWrapGapps = true;

  postInstall = ''
    mkdir $out/bin
    if [ -d "${gst_all_1.gstreamer }/bin" ]; then
      for file in ${gst_all_1.gstreamer}/bin/*; do
        ln -sf "$file" "$out/bin/"
      done
    fi
    if [ -d "${gst_all_1.gst-plugins-rs}/bin" ]; then
      for file in ${gst_all_1.gst-plugins-rs}/bin/*; do
        ln -sf "$file" "$out/bin/"
      done
    fi
    if [ -d "${v4l-utils}/bin" ]; then
      for file in ${v4l-utils}/bin/*; do
        ln -sf "$file" "$out/bin/"
      done
    fi

    wrapGApp "$out/lib/cameras3/cameras3_streamer_service"\
      --prefix GST_PLUGIN_SYSTEM_PATH_1_0 : "${gst_all_1.gst-plugins-base}/lib/gstreamer-1.0\
        :${gst_all_1.gst-plugins-good}/lib/gstreamer-1.0\
        :${gst_all_1.gst-plugins-bad}/lib/gstreamer-1.0\
        :${gst_all_1.gst-plugins-ugly}/lib/gstreamer-1.0\
        :${gst_all_1.gst-libav}/lib/gstreamer-1.0\
        :${gst_all_1.gst-vaapi}/lib/gstreamer-1.0\
        :${gst_all_1.gst-plugins-rs}/lib/gstreamer-1.0\
        :${gst-bridge}/lib/gstreamer-1.0"
    if [ -f "$out/lib/cameras3/.cameras3_streamer_service-wrapped" ]; then
      mv "$out/lib/cameras3/.cameras3_streamer_service-wrapped" "$out/lib/cameras3/cameras3_streamer_service"
    fi
  '';

}
