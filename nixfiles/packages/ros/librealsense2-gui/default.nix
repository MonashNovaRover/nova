{ lib
, fetchpatch
, librealsense2
, mesa
, gtk3
, curl
}:

librealsense2.overrideAttrs ({ version, patches ? [ ], buildInputs ? [ ], cmakeFlags ? [ ], preInstall ? "", ... }: {
  patches = patches ++ lib.optionals (lib.versionAtLeast version "2.49.0") [
    (fetchpatch {
      url = "https://github.com/IntelRealSense/librealsense/commit/3d04731c858984fe2de7e96bd8a8e280717c3a8d.patch";
      revert = true;
      hash = "sha256-We8KM1X8e8m1sJD1ZTQ/LSook79biaYkJaeEgueW7eU=";
    })
  ];

  buildInputs = buildInputs ++ [ mesa gtk3 curl ];

  cmakeFlags = cmakeFlags ++ [
    "-DBUILD_EXAMPLES=ON"
    "-DBUILD_GRAPHICAL_EXAMPLES=true"
    "-DDBUILD_GLSL_EXTENSIONS=true"
    "-DCHECK_FOR_UPDATES=false"
  ];
})
