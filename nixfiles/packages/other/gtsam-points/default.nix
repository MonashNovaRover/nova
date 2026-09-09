{
  stdenv,
  cmake,
  fetchgit,
  boost,
  eigen,
  gtsam,
  llvmPackages
}:

stdenv.mkDerivation {
  pname = "gtsam-points";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/koide3/gtsam_points";
    rev = "76437e4ddb03df98a4f8a875c8de28edb0394c7c";
    hash = "sha256-6rm/UFJ+4YZ8Km8ymIXaSsgSp549atmzLTIoPD2jBvo=";
  };

  nativeBuildInputs = [ cmake ];

  buildInputs = [ boost eigen gtsam llvmPackages.openmp ];
}
