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
  version = "0.1.11";

  src = fetchFromGitHub {
    owner = "hacker1024";
    repo = "JCAN";
    rev = "cf558af1753efe7c23b3c03b5e0aacfce81d5e14";
    hash = "sha256-KLY1n2dlwolXTTnt5eB6a/p86DTy64ie3Eb/foOLCxY=";
  };
  sourceRoot = "source/jcan-python";

  cargoDeps = rustPlatform.fetchCargoTarball {
    inherit src;
    name = "${pname}-${version}";
    hash = "sha256-krjuvoDLM8GQGlNt3+FKFGW0MK1oZwxaFxJ+lnEe5I4=";
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
