{ config, lib, pkgs, ... }:

let
  cfg = config.nova.laptops.intel;
  cfg-new = config.nova.laptops.intel-new;
  cfg-old = config.nova.laptops.intel-old;
in
{
  options.nova.laptops.intel.enable = lib.mkEnableOption "Nova Rover base station laptop intel cpu configuration";

  options.nova.laptops.intel-new.enable = lib.mkEnableOption "Nova Rover base station laptop new intel cpu configuration";

  options.nova.laptops.intel-old.enable = lib.mkEnableOption "Nova Rover base station laptop old intel cpu configuration";

  config = lib.mkIf cfg.enable {
    # CPU
    hardware.cpu.intel.updateMicrocode = lib.mkDefault config.hardware.enableRedistributableFirmware;
     
    # GPU https://github.com/intel/libvpl?tab=readme-ov-file#dispatcher-behavior-when-targeting-intel-gpus
    hardware.graphics = {
      enable = lib.mkDefault true;
      extraPackages = with pkgs; [
        intel-vaapi-driver
        intel-ocl
        intel-compute-runtime
        intel-media-driver
      ]
      ++ lib.optional cfg-new.enable vpl-gpu-rt
      ++ lib.optional cfg-old.enable intel-media-sdk;
    };
  };
}
