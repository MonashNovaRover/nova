{ 
  stdenv, 
  fetchgit, 
  cmake, 
  fetchpatch, 
}:

stdenv.mkDerivation {
  pname = "livox-sdk2";
  version = "0.0.0";

  src = fetchgit {
    name = "livox-sdk2-source";
    url = "https://github.com/Livox-SDK/Livox-SDK2";
    rev = "6a940156dd7151c3ab6a52442d86bc83613bd11b";
    hash = "sha256-NGscO/vLiQ17yQJtdPyFzhhMGE89AJ9kTL5cSun/bpU=";
  };

  nativeBuildInputs = [ cmake ];

  patches = [
    (fetchpatch {
      url = "https://patch-diff.githubusercontent.com/raw/Livox-SDK/Livox-SDK2/pull/99.patch";
      # revert = true;
      hash = "sha256-/lrLO8jeZaO8+MCVAV0olTIdS9kdrf7hPZz9fHqAOyU=";
    })
  ];
}