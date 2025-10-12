# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    pkgs,
    ansi,
    rover,
    mast,
    dir,
    base-terminal,
    rover-terminal,
    mast-terminal,
}:

let 
  mast-check = if mast == null then throw "You must provide a target for mast ssh commands.\nUsage: nix-shell shell.nix --argstr mast <user@ip>\ne.g nix-shell shell.nix --argstr mast nova@10.0.0.11" else "echo Loading...";
  cmds = {
    base.teleop = "launch-teleop";
    base.rviz = "launch-rviz";
    rover.gps = "launch-gps";
    rover.control = "launch-control";
    rover.camera = "launch-oaks";
    rover.software = "launch-auto-software";
    rover.rtabmap = "launch-rtabmap";
    rover.localization = "launch-localization";
    rover.navigation = "ros2 launch auto_bringup navigation.launch.py nav2_params_dir:=/home/nova/nova/src/ros/rover/auto/auto_bringup/params/nav2_arc";
    mast.gps = "ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";
  };
in

# note i followed the running the (URC) auto stack guide which im pretty sure is now out of date
# havent put in the appropriate arguments for arch and urc respectively
# https://www.notion.so/Running-the-Auto-Stack-234b71396171801eb667cbc884e3b13b
{
  arch = pkgs.mkShell {
    shellHook = ''
      ${mast-check}
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}ARCh Auto${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover}${ansi.nc} and mast at ${ansi.light-purple}${mast}${ansi.nc}... Running in ${ansi.orange}${dir}${ansi.nc}"
      ssh-copy-id ${rover}
      ssh-copy-id ${mast}
      ${base-terminal "Base:Rviz" cmds.base.rviz} \
      & ${rover-terminal "Rover:RTabMap" cmds.rover.rtabmap} \
      & ${rover-terminal "Rover:Localization" cmds.rover.localization} \
      & ${rover-terminal "Rover:Control" cmds.rover.control} \
      & ${rover-terminal "Rover:Camera" cmds.rover.camera} \
      & ${rover-terminal "Rover:Navigation" cmds.rover.navigation} \
      exit 0
    '';
  };

  urc = pkgs.mkShell {
    shellHook = ''
      ${mast-check}
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}URC Auto${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover}${ansi.nc} and mast at ${ansi.light-purple}${mast}${ansi.nc}... Running in ${ansi.orange}${dir}${ansi.nc}"
      ssh-copy-id ${rover}
      ssh-copy-id ${mast}
      ${base-terminal "Base:Teleop" cmds.base.teleop} \
      & ${base-terminal "Base:Rviz" cmds.base.rviz} \
      & ${rover-terminal "Rover:GPS" cmds.rover.gps} \
      & ${rover-terminal "Rover:Control" cmds.rover.control} \
      & ${rover-terminal "Rover:Camera" cmds.rover.camera} \
      & ${rover-terminal "Rover:Software" cmds.rover.software} \
      & ${mast-terminal "Mast:GPS" cmds.mast.gps}
      exit 0
    '';
  };
}