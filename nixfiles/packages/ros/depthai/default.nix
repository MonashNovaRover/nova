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
  version = "2.23.0";
  buildType = "prebuilt";

  src = fetchurl {
    # http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-${rosDistro}-depthai
    humble = {
      x86_64-linux = {
        url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-humble-depthai/ros-humble-depthai_${version}-1jammy.20240217.032335_amd64.deb";
        hash = "sha256-1wO2PnimBDY/sJlNHIXSGGR5gaGbqkDElDaUy3rRvpM=";
      };
      aarch64-linux = {
        url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-humble-depthai/ros-humble-depthai_${version}-1jammy.20240217.025930_arm64.deb";
        hash = "sha256-+gdMfyIvzpR4B2O6jwDIUmCNX6TB2vznzmZTOyV1W5o=";
      };
    };
  }.${ros-environment.rosDistro}.${hostPlatform.system}
    or (throw "There are no DepthAI Core hashes for ${ros-environment.rosDistro} on ${hostPlatform.system}.");

  nativeBuildInputs = [ dpkg autoPatchelfHook ];
  buildInputs = [ stdenv.cc.cc.lib opencv ];

  dontBuild = true;

  installPhase = ''
    mkdir -p "$out"
    cp -r opt/ros/${ros-environment.rosDistro}/* "$out"
    mv "$out/lib/${stdenv.hostPlatform.system}-gnu/cmake" "$out/lib"
  '';

  preFixup = ''
    chmod +x "$out"/lib/cmake/*/dependencies/bin/*

    patchelf \
      --replace-needed libopencv_core.so.4.5d libopencv_core.so \
      --replace-needed libopencv_imgproc.so.4.5d libopencv_imgproc.so \
      "$out"/lib/${stdenv.hostPlatform.system}-gnu/libdepthai-opencv.so
    
    for f in depthaiTargets depthaiTargets-none; do
      substituteInPlace "$out/lib/cmake/depthai/$f.cmake" \
        --replace "\''${_IMPORT_PREFIX}" "$out"
    done

    substituteInPlace "$out/share/depthai/local_setup.sh" \
      --replace /opt/ros/${ros-environment.rosDistro} "$out"
  '';
}
