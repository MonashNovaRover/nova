# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    base,
    rover,
    mast,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  arch-setup = {
    pre = pre-shell {payload-name="Auto ARCh"; need-rover=true;};
    terminals = [
      {name = "Base:Rviz"; platform=base; cmd="ros2 launch auto_bringup rviz.launch.py";}
      {name = "Rover:RTabMap"; platform=rover; cmd="ros2 launch auto_bringup rtabmap.launch.py";}
      {name = "Rover:Localization"; platform=rover; cmd="ros2 launch auto_bringup localization.launch.py";}
      {name = "Rover:Control"; platform=rover; cmd="launch-control";} # needs to be changed
      {name = "Rover:Camera"; platform=rover; cmd="ros2 launch auto_bringup camera.launch.py";}
      {name = "Rover:Navigation"; platform=rover; cmd="ros2 launch auto_bringup navigation.launch.py nav2_params_dir:=/home/nova/nova/src/ros/rover/auto/auto_bringup/params/nav2_arc";}
    ];
    post = post-shell;
  };

  urc-setup = {
    pre = pre-shell {payload-name="Auto URC"; need-rover=true; need-mast=true;};
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="launch teleop_drive_joy teleop.launch.py";}
      {name="Base:Rviz"; platform=base; cmd="ros2 launch auto_bringup rviz.launch.py";}
      {name="Rover:GPS"; platform=rover; cmd="ros2 launch nova_bringup gps_rover.launch.py";}
      {name="Rover:Auto Drive"; platform=rover; cmd="ros2 launch drive_bringup drive.launch.py auto:=True";}
      {name="Rover:Camera"; platform=rover; cmd="ros2 launch auto_bringup camera.launch.py";}
      {name="Rover:Software (Localization, Navigation, Yolo)"; platform=rover; cmd="ros2 launch auto_bringup software.launch.py";}
      {name="Mast:GPS"; platform=mast; cmd="ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";}
    ];
    post = post-shell;
  };
in
{
  auto-arch = bashBuilder arch-setup "run-auto-arch";
  auto-urc = bashBuilder urc-setup "run-auto-urc";
}