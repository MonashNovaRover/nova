{ stdenv
, automake
, autoconf
, libtool
, libpcap
, fetchFromGitHub
}:
stdenv.mkDerivation {
  name = "ptpd2";
  src = fetchFromGitHub {
    owner = "ptpd";
    repo = "ptpd";
    rev = "1ec9e650b03e6bd75dd3179fb5f09862ebdc54bf";
    hash = "sha256-jLBEo7LoYRA2ZLQQvhVXIlaqEqcRx3l8MRV4jF6dYFM=";
  };

  buildInputs = [
    automake
    autoconf
    libtool
    libpcap
  ];

  buildPhase = ''
    runHook preBuild

    autoreconf -vi
    ./configure

    make

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p $out/bin
    cp src/ptpd2 $out/bin/ptpd2

    runHook postInstall
  '';
}

