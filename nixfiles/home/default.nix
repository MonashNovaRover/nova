{
  imports = [
    ./branding
    ./desktop
    ./nova
    ./workspace
  ];

  nixpkgs.config.allowUnfree = true;
}
