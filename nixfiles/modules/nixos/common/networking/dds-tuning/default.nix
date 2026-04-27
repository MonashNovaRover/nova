{
  lib,
  config,
  ...
}:

let
  cfg = config.nova.networking.ddsTuning;
in
{
  options.nova.networking.ddsTuning.enable = lib.mkEnableOption "ROS2 DDS network tuning";

  config = lib.mkIf cfg.enable {
    boot.kernel.sysctl = {
      "net.ipv4.ipfrag_time" = 3; # seconds
      "net.ipv4.ipfrag_high_thresh" = 134217728; # 128 MB
      "net.core.rmem_max" = 67108864; # 64 MB
    };

    environment.sessionVariables = {
      CYCLONEDDS_URI = "file://${./cyclonedds.xml}";
    };
  };
}
