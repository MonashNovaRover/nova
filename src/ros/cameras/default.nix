{
  rosPackages = pkgs: with pkgs; {
    nova-cameras = callPackage ./cameras2++ { };
  };
}