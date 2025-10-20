{ config, pkgs, lib, ... }:

let
  cfg = config.nova.mocking.cameras;
in
{
  options.nova.mocking.cameras = {
    enable = lib.mkEnableOption "fake cameras";
    count = lib.mkOption {
      type = with lib.types; ints.positive;
      description = "The number of fake cameras to create.";
      default = 1;
    };
    firstNumber = lib.mkOption {
      type = with lib.types; ints.unsigned;
      description = "The number of the first fake camera. Additional cameras will be numbered sequentially.";
      default = 10;
    };
    specs = {
      width = lib.mkOption {
        type = with lib.types; ints.positive;
        description = "The width of the fake camera's video stream.";
        default = 1280;
      };
      height = lib.mkOption {
        type = with lib.types; ints.positive;
        description = "The height of the fake camera's video stream.";
        default = 720;
      };
      framerate = lib.mkOption {
        type = with lib.types; ints.positive;
        description = "The framerate of the fake camera's video stream.";
        default = 30;
      };
    };
  };

  config = lib.mkIf cfg.enable {
    boot = {
      extraModulePackages = with config.boot.kernelPackages; [ v4l2loopback ];
      kernelModules = [ "v4l2loopback" ];
      modprobeConfig.enable = true;
      extraModprobeConfig = ''
        # https://github.com/umlaeute/v4l2loopback#options
        options v4l2loopback \
          exclusive_caps=${builtins.concatStringsSep "," (map toString (lib.replicate cfg.count 1))} \
          video_nr=${builtins.concatStringsSep "," (map toString (lib.range cfg.firstNumber (cfg.firstNumber + cfg.count - 1)))} \
          card_label=${builtins.concatStringsSep "," (builtins.genList (i: ''"Nova mock camera ${toString (i + 1)}"'') cfg.count)}
      '';
    };

    services.udev.extraRules = ''
      # Match video devices     # v4l2loopback sets this # Generate a serial number      # Generate a path      NOVA000
      SUBSYSTEM=="video4linux", ATTR{max_openers}=="?*", ENV{ID_SERIAL}="camera$number", ENV{ID_PATH}="platform-6682000.xhci-usb-0:1.0:1:$number.0", TAG+="systemd"
    '';

    systemd.services.nova-mock-cameras =
      let
        deviceUnits = builtins.genList (i: "dev-video${toString (cfg.firstNumber + i)}.device") cfg.count;
      in
      {
        description = "Nova mock camera video";
        wantedBy = deviceUnits;
        bindsTo = deviceUnits;
        after = deviceUnits;
        path = with pkgs; [ gst_all_1.gstreamer ];
        environment.GST_PLUGIN_SYSTEM_PATH_1_0 = lib.makeSearchPathOutput "out" "lib/gstreamer-1.0" (with pkgs.gst_all_1; [
          gstreamer
          gst-plugins-base
          gst-plugins-good
        ]);
        script =
          let
            basePipeline = builtins.concatStringsSep " ! " [
              "videotestsrc pattern=ball foreground-color=0xF67216"
              (builtins.concatStringsSep "," (with cfg.specs; [
                "video/x-raw"
                "width=${toString width}"
                "height=${toString height}"
                "framerate=${toString framerate}/1"
              ]))
              "tee name=t"
            ];
            splitPipeline = builtins.concatStringsSep " " (builtins.genList
              (i: (builtins.concatStringsSep " ! " [
                "t."
                "queue"
                "textoverlay valignment=center halignment=center font-desc='Sans, 48' text='Test stream ${toString (i + 1)}'"
                "v4l2sink device=/dev/video${toString (cfg.firstNumber + i)}"
              ]))
              cfg.count);
          in
          ''
            gst-launch-1.0 --no-position ${basePipeline} ${splitPipeline}
          '';
      };
  };
}
