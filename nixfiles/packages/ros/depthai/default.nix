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
  version = "2.24.0";
  buildType = "prebuilt";

  src = fetchurl {
    # http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-${rosDistro}-depthai
    humble = {
      x86_64-linux = {
        url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-humble-depthai/ros-humble-depthai_${version}-1jammy.20240308.165504_amd64.deb";
        hash = "sha256-P5jsDRwQk08QqZfLp/HDbzedXm5sD9lRp9Qx0zsE+zQ=";
      };
      aarch64-linux = {
        url = "http://packages.ros.org/ros2/ubuntu/pool/main/r/ros-humble-depthai/ros-humble-depthai_${version}-1jammy.20240308.165854_arm64.deb";
        hash = "sha256-BCkGVO4ldSkJZMUKF9ScH2wnZ0NgFFIO/J2miqBu+F8=";
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
