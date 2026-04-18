# run mav + mission planner + custom script
{ 
    pkgs,
    base,
    pre-shell,
    post-shell,
    bashBuilder,
    base-nix
}:

let 
  # Optional: open Mission Planner web/docs after startup (can remove if not needed)
  delayed-open = "(echo \"Starting systems...\"; sleep 2) & \n";

  flag-args = {
    letter = "usb";
    variable = "usb_id";
    default = "1";
    description = "USB number";
  };

  setup = {
    pre = pre-shell { payload-name = "Drone Stack"; };

    terminals = [
      {
        name = "MAVProxy";
        platform = base;
        cmd = "~/Builds/drone/bin/mavproxy.py --master=/dev/ttyACM$usb_id --baudrate 115200 --out=udp:127.0.0.1:14550 --out=udp:127.0.0.1:14551 --logfile ~/mavlogs/mav.tlog";
      }

      {
        name = "Mission Planner";
        platform = base;

        # OPTION 1: Windows binary (adjust path)
        cmd = "sleep 5 && ~/Builds/master/bin/mission-planner";

        # OPTION 2 (WSL native Windows interop example):
        # cmd = "/mnt/c/Program\\ Files\\ (x86)/MissionPlanner/MissionPlanner.exe";
      }

      {
        name = "Drone GPS publisher";
        platform = base;
        cmd = "sleep 5 && ~/Builds/master/bin/ros2 run drone_gps drone_gps.py";
      }
    ];

    post = delayed-open + post-shell;

    buildInputs = [
      pkgs.xdg-utils
    ];

    flag-args = [ flag-args ];
  };
in
{
  mav-stack = bashBuilder setup "run-drone";
}