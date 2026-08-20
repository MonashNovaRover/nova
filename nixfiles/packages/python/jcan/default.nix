{ lib
, buildPythonPackage
, fetchFromGitHub
, rustPlatform
, cargo
, rustc
, setuptools-rust
, toml
}:

buildPythonPackage rec {
  pname = "jcan";
  version = "0.2.4";
  format = "setuptools";

  doCheck = false;

  src = fetchFromGitHub {
    owner = "leighleighleigh";
    repo = "JCAN";
    rev = "316faeeef0b9e75cbcb2fe63d4c575302a631554";
    hash = "sha256-qLjzSBTZLnVOtBtinSc2KGZYh5R2Im4VF0xc6EiANvQ=";
  };
  sourceRoot = "source/jcan_python";

  cargoDeps = rustPlatform.fetchCargoVendor {
    inherit src;
    name = "${pname}-${version}";
    lockFile = "${src}/Cargo.lock";
    hash = "sha256-fkEt/znKWmq7SzOIbePy/3hISwD6hfBGmjS1UPg8Nb4=";
  };

  nativeBuildInputs = [
    cargo
    rustPlatform.cargoSetupHook
    rustc
    setuptools-rust
    toml
  ];

  # jcan's cxx-generated callback.h uses uint16_t/uint32_t/uint64_t without
  # including <cstdint>. GCC 15 no longer pulls it in transitively, so we
  # force-include it for every C++ compilation unit in the build.
  env = {
    CXXFLAGS = "-include cstdint";
  };

  postPatch = ''
    chmod u+w ..
    ln -s ../Cargo.lock .
  '';
}
