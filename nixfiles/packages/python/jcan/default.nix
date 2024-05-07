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

  src = fetchFromGitHub {
    owner = "leighleighleigh";
    repo = "JCAN";
    rev = "316faeeef0b9e75cbcb2fe63d4c575302a631554";
    hash = "sha256-qLjzSBTZLnVOtBtinSc2KGZYh5R2Im4VF0xc6EiANvQ=";
    
  };
  sourceRoot = "source/jcan-python";

  cargoDeps = rustPlatform.fetchCargoTarball {
    inherit src;
    name = "${pname}-${version}";
    hash = "sha256-7rVyb4FMdW/nNsbWmRYTC7Hge4g4m8zTqHEQd0rrAMg=";
  };

  nativeBuildInputs = [
    cargo
    rustPlatform.cargoSetupHook
    rustc
    setuptools-rust
    toml
  ];

  postPatch = ''
    chmod u+w ..
    ln -s ../Cargo.lock .
  '';
}
