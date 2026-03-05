{ 
    base,
    base-nix,
    base-window,
    rover,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  task-name = "ec";
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true;};
    terminals = [
      {name = "Base:ecTeleop"; platform=base; cmd="./ros2 launch teleop_ec teleop.launch.py log_inputs:=true";}
      {name = "Rover:EC"; platform=rover; cmd="./ros2 launch nova_bringup ec_rover.launch.py";}
      {name = "Rover:Cameras"; platform=rover; cmd="cameras-orin payload:=ec";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; };
    terminals = [
      {name="Base:driveTeleop"; platform=base; cmd="./ros2 launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:Drive"; platform=rover; cmd="./ros2 launch drive_bringup drive.launch.py";}
      {name="Base:Reolink"; platform=base-nix "nix-shell -p ffmpeg"; cmd="reolink";}
      {name="Base:Reolink-ctl"; platform=base-window-nix "nix-shell -p ffmpeg"; cmd="~/Builds/master/bin/reolink-ctl";}
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
  ec-one = bashBuilder one (task-name+"-one");
  ec-two = bashBuilder two (task-name+"-two");
  ec-combined = bashBuilder combined (task-name+"-combined");
}