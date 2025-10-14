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
    pre-shell = pre-shell {payload-name="Auto ARCh"; need-rover=true;};
    terminals = [
      {name = "Base:Rviz"; platform=base; cmd="launch-rviz";}
      {name = "Rover:RTabMap"; platform=rover; cmd="launch-rtabmap";}
      {name = "Rover:Localization"; platform=rover; cmd="launch-localization";}
      {name = "Rover:Control"; platform=rover; cmd="launch-control";}
      {name = "Rover:Camera"; platform=rover; cmd="launch-oaks";}
      {name = "Rover:Navigation"; platform=rover; cmd="ros2 launch auto_bringup navigation.launch.py nav2_params_dir:=/home/nova/nova/src/ros/rover/auto/auto_bringup/params/nav2_arc";}
    ];
    post-shell = post-shell;
  };

  urc-setup = {
    pre = pre-shell {payload-name="Auto URC"; need-rover=true; need-mast=true;};
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="launch-teleop";}
      {name="Base:Rviz"; platform=base; cmd="launch-rviz";}
      {name="Rover:GPS"; platform=rover; cmd="launch-gps";}
      {name="Rover:Auto Drive"; platform=rover; cmd="launch-auto-drive";}
      {name="Rover:Camera"; platform=rover; cmd="launch-oaks";}
      {name="Rover:Software (Localization, Navigation, Yolo)"; platform=rover; cmd="launch-auto-software";}
      {name="Mast:GPS"; platform=mast; cmd="ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";}
    ];
    post = post-shell;
  };
in
{
  arch = bashBuilder arch-setup "run-auto-arch";
  urc = bashBuilder urc-setup "run-auto-urc";
}