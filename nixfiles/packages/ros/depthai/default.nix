{ hostPlatform
, stdenv
, fetchurl
, buildRosPackage
, ros-environment
, dpkg
, autoPatchelfHook
, opencv
}:

buildRosPackage rec {
  pname = "depthai";
  version = "2.26.1";
  buildType = "prebuilt";

  src = fetchurl {
    # We use a single ROS distro release regardless of the selected ROS distro,
    # because the DepthAI SDK does not actually use ROS at all - it is just
    # built with the ROS build system and shipped with ROS distributions.
    #
    # Generally, the latest distro should be used, as it is most likely to be
    # ABI-compatible with the dependencies in Nixpkgs.
    #
    # http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-jazzy-depthai
    x86_64-linux = {
      url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-jazzy-depthai/ros-jazzy-depthai_${version}-1noble.20240702.040046_amd64.deb";
      hash = "sha256-6JXW/4Bd9EWGkKm2b84zmYMUBCFxpecgn/DC807aJRw=";
    };
    aarch64-linux = {
      url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-jazzy-depthai/ros-jazzy-depthai_${version}-1noble.20240702.040535_arm64.deb";
      hash = "sha256-eiZiLvgsAP5KPgHGMLsBn6vSaur6rPCX0N747/COFjI=";
    };
  }.${hostPlatform.system} or (throw "There are no DepthAI Core hashes for ${hostPlatform.system}.");

  nativeBuildInputs = [ dpkg autoPatchelfHook ];
  buildInputs = [ stdenv.cc.cc.lib opencv ];

  dontBuild = true;

  installPhase = ''
    mkdir -p "$out"
    cp -r opt/ros/jazzy/* "$out"
    mv "$out/lib/${stdenv.hostPlatform.system}-gnu/cmake" "$out/lib"
  '';

  preFixup = ''
    chmod +x "$out"/lib/cmake/*/dependencies/bin/*

    patchelf \
      --replace-needed libopencv_core.so.406 libopencv_core.so \
      --replace-needed libopencv_imgproc.so.406 libopencv_imgproc.so \
      "$out"/lib/${stdenv.hostPlatform.system}-gnu/libdepthai-opencv.so
    
    for f in depthaiTargets depthaiTargets-none; do
      substituteInPlace "$out/lib/cmake/depthai/$f.cmake" \
        --replace-fail "\''${_IMPORT_PREFIX}" "$out"
    done
  '';
}
