{ hostPkgs, novaPkgs }:

let
  nixos-lib = import (hostPkgs.path + "/nixos/lib") { };

  runTest = module: nixos-lib.runTest {
    imports = [ module ];

    inherit hostPkgs;
    node.pkgs = novaPkgs;
    node.pkgsReadOnly = false;

    defaults = { pkgs, ... }: {
      # Protect machines from external influence.
      virtualisation.restrictNetwork = true;

      # Manually configure FastDDS.
      # TODO: When https://github.com/eProsima/Fast-DDS/pull/3973 is available,
      # inter-VM interfaces can be whitelisted.
      environment.sessionVariables.FASTRTPS_DEFAULT_PROFILES_FILE = pkgs.writeText "profiles.xml" ''
        <?xml version="1.0" encoding="UTF-8" ?>
        <dds>
            <profiles xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">
                <transport_descriptors>
                    <transport_descriptor>
                        <transport_id>CustomTransport</transport_id>
                        <type>UDPv4</type>
                    </transport_descriptor>
                </transport_descriptors>

                <participant profile_name="participant_profile_ros2" is_default_profile="true">
                    <rtps>
                        <name>profile_for_ros2_context</name>
                        <useBuiltinTransports>false</useBuiltinTransports>
                        <userTransports>
                            <transport_id>CustomTransport</transport_id>
                        </userTransports>
                    </rtps>
                </participant>
            </profiles>
        </dds>
      '';
    };

    nodes =
      let
        novaCommon = { config, lib, ... }: {
          imports = [ ../modules/nixos ];

          # Use the shared profile, as all team devices do.
          nova.profile = "shared";

          # Lock in the UID for stability in tests.
          users.users.nova.uid = 1000;

          # Disable the Nova substituter to avoid providing a password.
          # There is no need for substitutions in the VM anyway: Everything is
          # built beforehand by the host.
          nova.substituters.nova.enable = false;

          # Many integration tests will not want everything to run automatically.
          nova.workspace.enable = lib.mkDefault false;
          environment.systemPackages = [ config.nova.workspace.package ];

          # Disable the desktop by default.
          nova.desktop.enable = lib.mkOverride 900 false;

          # Enable workspace shell completions.
          environment.interactiveShellInit = ''
            eval "$(mk-workspace-shell-setup)"
          '';

          # Log in automatically.
          services.getty.autologinUser = config.users.users.nova.name;
          services.xserver.displayManager.autoLogin = {
            enable = true;
            user = config.users.users.nova.name;
          };

          # https://github.com/NixOS/nixpkgs/issues/103746#issuecomment-945091229
          systemd.services."getty@tty1".enable = lib.mkIf config.services.xserver.enable false;
          systemd.services."autovt@tty1".enable = lib.mkIf config.services.xserver.enable false;

          # The tests do not typically keep state, so their state version can
          # always be the latest.
          home-manager.users.nova.home.stateVersion = lib.mkDefault lib.trivial.release;
        };
      in
      {
        rover = { pkgs, lib, ... }: {
          imports = [ novaCommon ];

          virtualisation = {
            cores = 2;
            memorySize = lib.mkDefault (2 * 1024); # ROS is not very efficient!
          };

          # Use a similar kernel version to the rover. JetPack 5 uses Linux 5.10.
          boot.kernelPackages = pkgs.linuxKernel.packages.linux_5_10;
        };

        base = ({ config, lib, ... }: {
          imports = [ novaCommon ];

          virtualisation = {
            cores = 2;
            memorySize = lib.mkDefault (4 * 1024); # GNOME...
            qemu.options = lib.optionals config.nova.desktop.wayland.enable [ "-vga virtio" ];
          };

          services.xserver = {
            enable = true;
            xrandrHeads = [{
              output = "Virtual-1";
              monitorConfig = ''
                Option "PreferredMode" "1360x768"
              '';
            }];
          };

          nova.desktop = {
            enable = true;
            wayland.enable = false; # Wayland is not as easy to control remotely as Xorg.
          };
        });
      };

    _module.args.testScriptCommon = { nodes, ... }:
      let
        dbus = "unix:path=/run/user/${toString nodes.base.users.users.nova.uid}/bus";
        xauthority = "/run/user/${toString nodes.base.users.users.nova.uid}/gdm/Xauthority";
      in
      ''
        # Graphical features
        # Based on GNOME test: https://github.com/NixOS/nixpkgs/blob/bccd3c82dbbbad83d34a4bb286653e44bdf8fc70/nixos/tests/gnome-xorg.nix#L45

        def init_graphical() -> None:
            base.wait_for_x()
            base.wait_for_unit("default.target", "${nodes.base.users.users.nova.name}")
            base.wait_for_file("${xauthority}")
            base.succeed("xauth merge '${xauthority}'")

        def run_graphical(command: str) -> str:
            return f"su - '${nodes.base.users.users.nova.name}' -c 'DBUS_SESSION_BUS_ADDRESS=\"${dbus}\" XAUTHORITY=\"${xauthority}\" {command}'"
      '';
  };
in
import ./tests.nix { inherit runTest; }
