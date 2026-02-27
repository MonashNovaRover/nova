{ config, pkgs, lib, ... }:

let
  cfg = config.nova.substituters;
in
{
  options.nova.substituters = {
    ros.enable = lib.mkEnableOption "the upstream nix-ros-overlay binary cache" // { default = true; };
    nova = {
      enable = lib.mkEnableOption "the Nova Rover binary cache" // { default = true; };
      url = lib.mkOption {
        type = with lib.types; str;
        description = "The URL of the Nova Rover binary cache";
        default = "https://hydra.novarover.space";
      };
      password = lib.mkOption {
        type = with lib.types; str;
        description = "The password for the Nova Rover binary cache";
        default = builtins.readFile ../../../../secrets/hydra-password.txt;
      };
      publicKey = lib.mkOption {
        type = with lib.types; str;
        description = "The public key of the Nova Rover binary cache";
        default = builtins.readFile ../../../../secrets/hydra-public-key.txt;
      };
    };
    nvidia.enable = lib.mkEnableOption "the upstream nvidia binary caches";
  };

  config.nix.settings = lib.mkMerge [
    (lib.mkIf cfg.ros.enable {
      substituters = [ "https://ros.cachix.org" ];
      trusted-public-keys = [ "ros.cachix.org-1:dSyZxI8geDCJrwgvCOHDoAfOm5sV1wCPjBkKL+38Rvo=" ];
    })
    (lib.mkIf cfg.nova.enable {
      substituters = [ cfg.nova.url ];
      trusted-public-keys = [ cfg.nova.publicKey ];
      netrc-file = pkgs.writeText "nova-netrc" ''
        machine ${lib.removePrefix "https://" (lib.removePrefix "http://" cfg.nova.url)}
        login nova
        password ${cfg.nova.password}
      '';
    })
    (lib.mkIf cfg.nvidia.enable {
      substituters = [
        "https://cache.nixos-cuda.org"
        "https://cache.flox.dev"
      ];
      trusted-public-keys = [
        "cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E="
        "flox-cache-public-1:7F4OyH7ZCnFhcze3fJdfyXYLQw/aV7GEed86nQ7IsOs="
      ];
    })
  ];
}
