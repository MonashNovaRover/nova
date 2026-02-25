{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    programs.vscode.profiles.default = {
      extensions = [
        (pkgs.vscode-utils.buildVscodeMarketplaceExtension {
          mktplcRef = {
            publisher = "morningfrog";
            name = "urdf-visualizer";
            version = "4.8.0";
            sha256 = "sha256-GYztLHln5NJTTAdqK2P9k+Z06N3y9am0a+XGcOirndQ=";
          };
        })
      ];

      userSettings = {
        "urdf-visualizer.packages" = {
            "rover_description" = "/home/nova/nova/src/ros/rover/rover_description";
            "depthai_descriptions" = "/home/nova/Builds/master/share/depthai_descriptions";
            "realsense2_description" = "/home/nova/Builds/master/share/realsense2_description";
        };
      };
    };
  };
}
