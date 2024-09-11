{ lib, ... }:

{
  options.nova.repos = lib.mkOption {
    type = with lib.types; nullOr (listOf path);
    description = lib.mdDoc ''The Nova Rover repositories to pass to the main Nix function. Leave unset to search in the default locations.'';
    default = null;
  };
}
