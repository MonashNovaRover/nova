{ config, lib, ... }:

let
  cfg = config.devices.laptop.amd;
in
{
  options.devices.laptop.amd.enable = lib.mkEnableOption "Nova Rover base station laptop amd cpu configuration";

  config = lib.mkIf cfg.enable {
    # CPU
    hardware.cpu.amd.updateMicrocode = lib.mkDefault config.hardware.enableRedistributableFirmware;
    boot.kernelParams = [ "amd_pstate=active" ];
    
    # GPU
    services.xserver.videoDrivers = lib.mkDefault [ "modesetting" ];

    hardware.graphics = {
      enable = lib.mkDefault true;
      enable32Bit = lib.mkDefault true;
    };

    hardware.amdgpu.initrd.enable = lib.mkDefault true;
  };
}
