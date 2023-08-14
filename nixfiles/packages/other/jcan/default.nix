{ lib
, testers
, validatePkgConfig
, fetchFromGitHub
, rustPlatform
}:

let
  self =
    rustPlatform.buildRustPackage {
      name = "jcan";

      src = fetchFromGitHub {
        owner = "hacker1024";
        repo = "JCAN";
        rev = "cf558af1753efe7c23b3c03b5e0aacfce81d5e14";
        hash = "sha256-KLY1n2dlwolXTTnt5eB6a/p86DTy64ie3Eb/foOLCxY=";
      };

      cargoHash = "sha256-clm96oC4hjrMCjPmyEip1114F2eq+FhWNHC3Ndmzl1E=";

      buildAndTestSubdir = "jcan";

      outputs = [ "out" "dev" ];

      nativeBuildInputs = [ validatePkgConfig ];

      postInstall = ''
        mkdir -p "$dev/include/jcan"
        cp target/cxxbridge/jcan/src/lib.rs.h "$dev/include/jcan/jcan.h"
        cp target/cxxbridge/jcan/src/lib.rs.cc "$dev/include/jcan/jcan.cc"
        cp target/cxxbridge/rust/cxx.h "$dev/include/jcan"
        cp jcan/include/* "$dev/include/jcan"
        cp jcan/src/callback.cc "$dev/include/jcan"
        find "$dev/include/jcan" -type f \
          -exec sed -i 's/#include "jcan\/include/#include "jcan/g' {} \; \
          -exec sed -i 's/src\/lib.rs/jcan/g' {} \;

        mkdir -p "$dev/share/pkgconfig"
        cat > "$dev/share/pkgconfig/jcan.pc" << EOF
        Name: JCAN
        Description: An easy-to-use SocketCAN library for Python and C++, built in Rust.
        Version: 0.1.11
        Libs: -L$out/lib -ljcan
        Cflags: -I$dev/include/jcan
        EOF
      '';

      passthru.tests.pkg-config = testers.testMetaPkgConfig self;

      meta.pkgConfigModules = [ "jcan" ];
    };
in
self
