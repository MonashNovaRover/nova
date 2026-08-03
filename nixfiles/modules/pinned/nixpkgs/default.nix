let
  revisions = builtins.fromJSON (builtins.readFile ../../revisions.json);
  nixpkgs = builtins.fetchTarball {
    url = "https://github.com/nixos/nixpkgs/archive/${revisions.nixpkgs.rev}.tar.gz";
    sha256 = revisions.nixpkgs.hash;
  };
in
  import nixpkgs
