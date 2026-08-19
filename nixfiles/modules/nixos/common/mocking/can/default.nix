{ config, pkgs, lib, ... }:

let
  cfg = config.nova.mocking.can;
in
{
  options.nova.mocking.can = {
    enable = lib.mkEnableOption "fake can devices";
    systems = lib.mkOption {
      # TODO: probably should be a list of (system name, emulate, interface) but that's a lot of effort to organise.
      type = lib.types.str;
      description = "Arguments for can sleuth for what systems to load etc";
      default = "-e drive";
    };
    # TODO: configure vcan here?
    # TODO: allow injecting a different package in place of can sleuth
    file = lib.mkOption {
      type = lib.types.str;
      description = "The file for the state of the mock can devices to be reported. Use commands like `jq .J6.BLCMDEmulator.Pos /run/can_sleuth_state` to get values from there.";
      default = "/run/can_sleuth_state";
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services.nova-can-sleuth =
      {
        enable = true;
        wantedBy = ["default.target"];
        after = ["network.target"];
        description = "Nova mock can devices";
        script =
          let
            awk = "${pkgs.gawk}/bin/awk";
            can_sleuth = "${pkgs.nova.python3Packages.nova-can-sleuth}/bin/can_sleuth";
            ip = "${pkgs.iproute2}/bin/ip";
          in
          # The awk script overwrites the file every time it gets an update https://unix.stackexchange.com/a/356398
          ''
            #TODO: this is a bad way of doing it but i'm tired
            ${ip} link add dev can0 type vcan || :
            ${ip} link add dev can1 type vcan || :
            ${ip} link add dev can2 type vcan || :
            ${ip} link set up can0 || :
            ${ip} link set up can1 || :
            ${ip} link set up can2 || :
            ${can_sleuth} ${cfg.systems} -o json | ${awk} -vfile=${cfg.file} '{print $0 > file; close(file)}'
          '';
      };
  };
}
