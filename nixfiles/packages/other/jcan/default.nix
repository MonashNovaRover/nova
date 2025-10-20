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
        owner = "leighleighleigh";
        repo = "JCAN";
        rev = "316faeeef0b9e75cbcb2fe63d4c575302a631554";
        hash = "sha256-qLjzSBTZLnVOtBtinSc2KGZYh5R2Im4VF0xc6EiANvQ=";
      };

      cargoHash = "sha256-fkEt/znKWmq7SzOIbePy/3hISwD6hfBGmjS1UPg8Nb4=";

      buildAndTestSubdir = "jcan";

      outputs = [ "out" "dev" ];

      doCheck = false;

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
