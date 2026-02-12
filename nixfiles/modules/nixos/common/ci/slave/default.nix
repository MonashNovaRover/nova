{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.slave;
in
{
  options.nova.ci.slave = {
    enable = lib.mkEnableOption "CI slave services";

    remotePubSSHKey = lib.mkOption {
        type = with lib.types; str;
        description = lib.mdDoc "SSH pub key of the nixbuild user on the master.";
        default = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIC0Uce72ktg6CcPeMb94XhukecdBmuTnsUVyb2R/+wDe root@ci";
      };

  };

  config = lib.mkIf cfg.enable {
    nova.ci.common.enable = true;

    users.users.remotebuild = {
      isNormalUser = true;
      createHome = false;
      group = "remotebuild";
      openssh.authorizedKeys.keys = [ cfg.remotePubSSHKey ];
    };

    users.groups.remotebuild = {};
    nix.settings.trusted-users = [ "remotebuild" ];

    assertions = [
      {
        assertion = !config.nova.ci.master.enable;
        message = "The CI master and slave modules cannot be enabled simultaneously.";
      }
    ];
  };
}
