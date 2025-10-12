# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    pkgs,
    ansi,
    rover-ip,
    mast-ip,
    dir,
    base-terminal,
    rover-terminal,
    mast-terminal,
    make-terminals,
}:

let 
  mast-check = if mast-ip == null then throw "You must provide a target for mast ssh commands.\nUsage: nix-shell shell.nix --argstr mast <user@ip>\ne.g nix-shell shell.nix --argstr mast nova@10.0.0.11" else "echo Loading...";
  # aliases for input arguments
  base = base-terminal;
  rover = rover-terminal;
  mast = mast-terminal;
  
  # note i followed the running the auto stack guide which im pretty sure is now out of date
  # havent put in the appropriate arguments for arch and urc respectively
  # https://www.notion.so/Running-the-Auto-Stack-234b71396171801eb667cbc884e3b13b
  arch-setup = [
    {name = "Base:Rviz"; platform=base; cmd="launch-rviz";}
    {name = "Rover:RTabMap"; platform=rover; cmd="launch-rtabmap";}
    {name = "Rover:Localization"; platform=rover; cmd="launch-localization";}
    {name = "Rover:Control"; platform=rover; cmd="launch-control";}
    {name = "Rover:Camera"; platform=rover; cmd="launch-oaks";}
    {name = "Rover:Navigation"; platform=rover; cmd="ros2 launch auto_bringup navigation.launch.py nav2_params_dir:=/home/nova/nova/src/ros/rover/auto/auto_bringup/params/nav2_arc";}
  ];

  urc-setup = [
    {name="Base:Teleop"; platform=base; cmd="launch-teleop";}
    {name="Base:Rviz"; platform=base; cmd="launch-rviz";}
    {name="Rover:GPS"; platform=rover; cmd="launch-gps";}
    {name="Rover:Control"; platform=rover; cmd="launch-control";}
    {name="Rover:Camera"; platform=rover; cmd="launch-oaks";}
    {name="Rover:Software"; platform=rover; cmd="launch-auto-software";}
    {name="Mast:GPS"; platform=mast; cmd="ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";}
  ];
  
in

{
  arch = pkgs.mkShell {
    shellHook = ''
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}ARCh Auto${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover-ip}${ansi.nc}... Running in ${ansi.orange}${dir}${ansi.nc}..."
      ssh-copy-id ${rover-ip}
      ${make-terminals arch-setup}
      exit 0
    '';
  };

  urc = pkgs.mkShell {
    shellHook = ''
      ${mast-check}
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}URC Auto${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover-ip}${ansi.nc} and mast at ${ansi.light-purple}${mast-ip}${ansi.nc}... Running in ${ansi.orange}${dir}${ansi.nc}"
      ssh-copy-id ${rover-ip}
      ssh-copy-id ${mast-ip}
      ${make-terminals urc-setup}
      exit 0
    '';
  };
}