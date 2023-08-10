{ config, pkgs, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    hardware.opengl.extraPackages = with pkgs; [ intel-compute-runtime ];
    nixpkgs.config.cudaSupport = true;
    nix.settings = {
      substituters = [ "https://cuda-maintainers.cachix.org" ];
      trusted-public-keys = [ "cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E=" ];
    };
  };
}
