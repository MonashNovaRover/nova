{ config, lib, pkgs, ... }:

let
  cfg = config.nova.monitoring;
in
{
  options.nova.monitoring.enable = lib.mkEnableOption "Install monitoring tools";

  config = lib.mkIf cfg.enable {
    # Wireshark, USB bandwidth monitor, tcpdump
    programs = {
      wireshark.enable = true;
      usbtop.enable = true;
      tcpdump.enable = true;
    };

    # User groups for wireshark and tcpdump
    users.users.nova = {
      extraGroups = [ "wireshark" "pcap" ];
    };

    # Network bandwidth by process/application monitor
    environment.systemPackages = with pkgs; [
      nethogs
    ];

    # Security wrappers for tools that require elevated capabilities
    # Enables running usbtop and nethogs without sudo while still maintaining security
    security.wrappers.usbtop = {
      source = "${pkgs.usbtop}/bin/usbtop";
      capabilities = "cap_dac_read_search+ep";
      owner = "root";
      group = "root";
    };
    security.wrappers.nethogs = {
      source = "${pkgs.nethogs}/bin/nethogs";
      capabilities = "cap_net_admin,cap_net_raw,cap_dac_read_search,cap_sys_ptrace+ep";
      owner = "root";
      group = "root";
    };
  };
}
