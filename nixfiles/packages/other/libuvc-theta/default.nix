{ libuvc
, fetchFromGitHub
}:

libuvc.overrideAttrs ({ pname, patches ? [ ], ... }: {
  pname = "libuvc-theta";
  src = fetchFromGitHub {
    owner = "nickel110";
    repo = "libuvc";
    rev = "8b58a694e4cdedd6dc09031398e927c3092f1b70";
    hash = "sha256-sPc3mazgKMvLwn8mD0XzKeXrJcu+bleLQJ9zznkq5AE=";
  };
})
