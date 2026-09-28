{ libuvc
, fetchFromGitHub
}:

libuvc.overrideAttrs ({ pname, patches ? [ ], ... }: {
  pname = "libuvc-theta";
  src = fetchFromGitHub {
    owner = "ricohapi";
    repo = "libuvc-theta";
    rev = "e4c9786064bf120abf054df412c820b8813d27b0";
    hash = "sha256-5TMuu4rHKQtPDiXu3X+e6EfGPx+Yow121nkTtzxXk6E=";
  };
})
