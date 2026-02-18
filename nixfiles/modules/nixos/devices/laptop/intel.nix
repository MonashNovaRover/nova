{ config, lib, pkgs, ... }:

let
  cfg = config.devices.laptop.intel;
  cfg-new = config.devices.laptop.intel-new;
  cfg-old = config.devices.laptop.intel-old;
in
{
  options.devices.laptop.intel.enable = lib.mkEnableOption "Nova Rover base station laptop intel cpu configuration";

  options.devices.laptop.intel-new.enable = lib.mkEnableOption "Nova Rover base station laptop new intel cpu configuration";

  options.devices.laptop.intel-old.enable = lib.mkEnableOption "Nova Rover base station laptop old intel cpu configuration";

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
