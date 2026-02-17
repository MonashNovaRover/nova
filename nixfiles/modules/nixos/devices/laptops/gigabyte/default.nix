{ config, lib, ... }:

let
  cfg = config.nova.laptops.gigabyte;
in
{
  imports = [
    ./nvidia.nix
  ];

  options.nova.laptops.gigabyte.enable = lib.mkEnableOption "configuration for the Gigabyte";

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
