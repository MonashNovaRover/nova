{ stdenv
, fetchFromGitHub
, boost
, openssl
}:

stdenv.mkDerivation rec {
  pname = "stunserver";
  version = "d2d06a6d002d1e2fba312cf4993b08bb57a0346b";

  src = fetchFromGitHub {
    owner = "jselbie";
    repo = pname;
    rev = version;
    hash = "sha256-c/TGvksz9oZB4f1d4MEhAN8BxfLOnQOOLnZeErPmEzE=";
  };

  buildInputs = [ boost openssl ];

  installPhase = ''
    runHook preInstall
  
    mkdir -p $out/bin
    install -Dm755 stunclient $out/bin
    install -Dm755 stunserver $out/bin
  
    runHook postInstall
  '';
}
