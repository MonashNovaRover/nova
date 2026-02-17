{ config, lib, ... }:

let
  cfg = config.nova.laptops.metabox-new;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.metabox-new.enable = lib.mkEnableOption "configuration for the Metabox New";

  config = lib.mkIf cfg.enable {
    nova.laptops = {
      enable = true;
      intel.enable = true;
      intel-new.enable = true;
      nvidia.enable = true;
      performance.enable = true;
    };
  };
}
