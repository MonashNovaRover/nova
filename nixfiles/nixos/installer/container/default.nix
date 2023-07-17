{ name ? "nova", modules ? [ ] }:

{
  containers.${name} = {
    additionalCapabilities = [
      # https://github.com/jaraco/keyring/issues/477#issuecomment-750334406
      "CAP_IPC_LOCK"
    ];
    bindMounts = {
      "/tmp/.X11-unix" = {
        hostPath = "/tmp/.X11-unix";
        isReadOnly = false;
      };
      "/dev/dri" = {
        hostPath = "/dev/dri";
        isReadOnly = false;
      };
      "/dev/shm" = {
        hostPath = "/dev/shm";
        isReadOnly = false;
      };
    };
    config = { config, pkgs, lib, ... }: {
      imports = [ (import ../../.. { }).nixosModule ] ++ modules;

      config = lib.mkMerge [
        {
          services.xserver.enable = lib.mkDefault true;

          nova = {
            profile = lib.mkDefault "shared";
            substituters.nova.enable = lib.mkDefault false;
          };

          home-manager.nova.sharedModules = [{
            home.stateVersion = lib.mkDefault (lib.warn "home.stateVersion has not been set." lib.trivial.release);
          }];
        }

        (lib.mkIf config.services.xserver.enable {
          # Enable startx
          services.xserver = {
            displayManager.startx.enable = true;
          };

          # Enable graphics acceleration
          hardware.opengl = {
            enable = true;
            driSupport = true;
          };
        })

        (lib.mkIf (config.services.xserver.enable && config.nova.desktop.enable) {
          # Configure the desktop start script
          home-manager.sharedModules = [{
            home.file.".xinitrc".source = pkgs.writeScript "xinitrc" ''
              # Set up D-Bus
              # https://github.com/NixOS/nixpkgs/issues/94375
              export XDG_SESSION_TYPE=x11
              export XDG_CURRENT_DESKTOP=GNOME
              dbus-daemon --session --address="unix:path=$XDG_RUNTIME_DIR/bus" &
              systemctl --user import-environment DISPLAY XAUTHORITY
              dbus-update-activation-environment DISPLAY XAUTHORITY
              
              gnome-shell &
              sleep 2
              xrandr --size ''${SIZE:-800x600} &
              wait
            '';
          }];

          environment.systemPackages = with pkgs; [
            # The GNOME module leaves out certain packages that should be added
            # to the environment.
            ibus

            # Add a script to start a nested X11 session
            (pkgs.writeScriptBin "startx-nested" ''
              export DISPLAY=''${DISPLAY:-:0}
              exec startx -- "$(type -p Xephyr)" ''${NESTED_DISPLAY:-:1} -screen ''${SIZE:-800x600} -dpi ''${DPI:-96} +extension RANDR +extension DPMS +extension GLX
            '')
          ];
        })
      ];
    };
  };
}
