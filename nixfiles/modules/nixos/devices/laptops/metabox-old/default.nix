{ config, lib, ... }:

let
  cfg = config.nova.laptops.metabox-old;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.metabox-old.enable = lib.mkEnableOption "configuration for the Metabox Old";

  config = lib.mkIf cfg.enable {
    nova.laptops = {
      enable = true;
      intel.enable = true;
      intel-old.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
