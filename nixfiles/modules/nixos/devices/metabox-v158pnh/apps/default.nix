{ pkgs, ... }:

{
  environment.systemPackages = with pkgs; [
    (blender.override { cudaSupport = true; })
  ];
}
