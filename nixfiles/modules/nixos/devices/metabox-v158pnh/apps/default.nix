{ config, pkgs, lib, ... }:

{
  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    environment.systemPackages = with pkgs; [
      (blender.override { cudaSupport = true; })
    ];
  };
}
