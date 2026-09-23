{ 
  cmake, 
  fetchFromGitHub, 
  fetchpatch, 
  stdenv, 
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "livox-sdk2";
  version = "1.3.1";

  src = fetchFromGitHub {
    owner = "Livox-SDK";
    repo = "Livox-SDK2";
    rev = "v${finalAttrs.version}";
    hash = "sha256-XM2jhytXbLVd3jkeZrpxDjegPWPiXCaVQ3nYm1DD928=";
  };

  nativeBuildInputs = [ cmake ];

  cmakeFlags = [ "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" ];

  patches = [
    (fetchpatch {
      url = "https://patch-diff.githubusercontent.com/raw/Livox-SDK/Livox-SDK2/pull/99.patch";
      hash = "sha256-/lrLO8jeZaO8+MCVAV0olTIdS9kdrf7hPZz9fHqAOyU=";
    })
  ];
})