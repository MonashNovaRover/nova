{ lib
, ament-cmake
, buildRosPackage
, gobject-introspection
, gst_all_1
, gst-bridge
, libnice
, nova-camera-msgs
, pkg-config
, rclcpp
, std-msgs
, std-srvs
, systemd
, sysprof
, v4l-utils
, wrapGAppsNoGuiHook
, mesa
, libGL
, pciutils
}:

buildRosPackage {
  name = "nova-cameras";
  buildType = "ament_cmake";

  src = builtins.filterSource (path: type: baseNameOf path != "build") ./.;

  nativeBuildInputs = [ 
    ament-cmake
    pkg-config
    gst_all_1.gstreamer
    wrapGAppsNoGuiHook
    gobject-introspection
    sysprof                   # Build error
  ];

  buildInputs = [
    rclcpp
    std-msgs
    std-srvs
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

    mesa                      # GPU acceleration for x86
    libGL
    pciutils
  ];

  postInstall = ''
    mkdir $out/bin
    if [ -d "${gst_all_1.gstreamer }/bin" ]; then
      for file in ${gst_all_1.gstreamer}/bin/*; do
        ln -sf "$file" "$out/bin/"
      done
    fi
    if [ -d "${gst_all_1.gst-plugins-rs}/bin" ]; then
      ln -sf "${gst_all_1.gst-plugins-rs}/bin/gst-webrtc-signalling-server" "$out/bin/gst-webrtc-signalling-server"
    fi
    if [ -d "${v4l-utils}/bin" ]; then
      ln -sf "${v4l-utils}/bin/v4l2-ctl" "$out/bin/v4l2-ctl"
    fi
    '';

  preFixup = ''
    set +e
    # Detect GPU type (Intel or AMD)
    GPU_TYPE=$(lspci | grep -iE "VGA|3D" | grep -i -E "Intel|AMD" | awk '{print $0}')

    # Conditional block based on GPU type
    if echo "$GPU_TYPE" | grep -iq "AMD\|Intel"; then
      # AMD or Intel GPU detected
      wrapGApp "$out/lib/cameras/camera_streamer_service" \
        --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib" \
        --prefix GBM_BACKENDS_PATH : "${mesa}/lib/gbm" \
        --prefix LIBGL_DRIVERS_PATH : "${mesa}/lib/dri" \
        --prefix LIBVA_DRIVERS_PATH : "${mesa}/lib/dri" \
        --prefix __EGL_VENDOR_LIBRARY_FILENAMES : "${mesa}/share/glvnd/egl_vendor.d/50_mesa.json" \
        --prefix LD_LIBRARY_PATH : "${mesa}:${libGL}"
    else
      # If NVIDIA GPU is detected, use the default paths
      export GST_PLUGIN_PATH="${gst-bridge}/lib:$GST_PLUGIN_PATH"
      wrapGApp "$out/lib/cameras/camera_streamer_service"\
        --prefix GST_PLUGIN_PATH : "${gst-bridge}/lib"
    fi
    set -e
  '';
}
