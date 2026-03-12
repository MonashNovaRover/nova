{ 
    base,
    rover,
    pre-shell,
    post-shell,
    bashBuilder, 
    base-nix
}:

let 
  task-name = "sr";
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true;};
    terminals = [
      {name = "Base:scienceTeleop"; platform=base; cmd="./ros2 launch teleop_science teleop.launch.py";}
      {name = "Base:guiRosbridge"; platform=base; cmd="./ros2 launch rosbridge_server rosbridge_websocket_launch.xml";}
      {name = "Base:guiRun"; platform=base-nix "nix-shell ~/nova/src/../nixfiles -A pkgs.ros.nova-gui"; cmd= "gui-run";}
      {name = "Rover:science"; platform=rover; cmd="./ros2 launch science_bringup arc.launch.py";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; };
    terminals = [
      {name = "Base:driveTeleop"; platform=base; cmd="./ros2 launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:drive"; platform=rover; cmd="./ros2 launch drive_bringup drive.launch.py";}
      {name = "Rover:scienceCameras"; platform=rover; cmd="cameras-orin payload:=science-arc";}
      {name = "Rover:reolink"; platform=base-nix "nix-shell -p ffmpeg"; cmd="reolink";}
      {name = "Rover:reolinkCtl"; platform=base-nix"nix-shell -p ffmpeg"; cmd="~/Builds/master/bin/reolink-ctl";}

    ];
    post = post-shell;
  };

  combined = {
    pre = pre-shell {payload-name=task-name+" combined"; need-rover=true; };
    terminals = one.terminals ++ two.terminals;
    post = post-shell;
  };
in
{
  sr-one = bashBuilder one (task-name+"-one");
  sr-two = bashBuilder two (task-name+"-two");
  sr-combined = bashBuilder combined (task-name+"-combined");
}