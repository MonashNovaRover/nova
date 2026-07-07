{ config, pkgs, lib, ... }:

let
  cfg = config.nova.mocking.can;
in
{
  options.nova.mocking.can = {
    enable = lib.mkEnableOption "fake can devices";
    systems = lib.mkOption {
      # TODO: probably should be a list of (system name, emulate, interface) but that's a lot of effort to organise.
      type = with lib.types; string;
      description = "Arguments for can sleuth for what systems to load etc";
      default = "-e drive";
    };
    # TODO: configure vcan here?
    file = lib.mkOption {
      type = with lib.types; string;
      description = "The file for the state of the mock can devices to be reported. Use commands like `jq .J6.BLCMDEmulator.Pos /run/can_sleuth_state` to get values from there."
      default = "/run/can_sleuth_state";
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services.nova-can-sleuth =
      {
        description = "Nova mock can devices";
        path = with pkgs; [ gst_all_1.gstreamer ];
        script =
          let
            awk = "${pkgs.gawk}/bin/awk";
            can_sleuth = "${pkgs.python3Pakages.nova-can-sleuth}/bin/can_sleuth";
          in
          # The awk script overwrites the file every time it gets an update https://unix.stackexchange.com/a/356398
          ''
            ${can_sleuth} ${systems} -o json | ${awk} -vfile=${cfg.file} '{print $0 > file; close(file)}'
          '';
      };
  };
}
