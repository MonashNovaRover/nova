# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    pkgs ? import <nixpkgs> {}, 
    rover ? throw ''You must provide a target for rover ssh commands\nUsage: nix-shell shell.nix --argstr rover <user@ip>\ne.g nix-shell shell.nix --argstr target nova@10.0.0.11'',
    mast ? throw ''You must provide a target for mast ssh commands\nUsage: nix-shell shell.nix --argstr mast <user@ip>\ne.g nix-shell shell.nix --argstr target nova@10.0.0.11'',
    dir ? "/home/nova/Builds/master/bin"
}:

let 
  ansi = {
    light-red = ''\033[1;31m'';
    orange = ''\033[0;33m'';
    yellow = ''\033[1;33m'';
    light-green = ''\033[1;32m'';
    light-purple = ''\033[1;35m'';
    nc = ''\033[0m''; # no colour
  };
  # these run a single command, will need to make a custom line for multiple commands
  base-terminal = name: cmd: ''ptyxis --tab --title="${name}" -x "bash -c '${dir}/${cmd}; echo -e Command: ${ansi.yellow}${dir}/${cmd}${ansi.nc}; exec bash'"'';
  rover-terminal = name: cmd: ''ptyxis --tab --title="${name}" -x "bash -c 'ssh -t ${rover} \"${dir}/${cmd}; echo -e Command: ${ansi.yellow}${dir}/${cmd}${ansi.nc}; exec bash -l\"; exec bash'"'';
  cmds = {
    base.teleop = "ros2 launch teleop_drive_joy teleop.launch.py";
    base.rviz = "ros2 launch auto_bringup rviz.launch.py";
    rover.gps = "ros2 launch nova_bringup gps_rover.launch.py";
    rover.control = "ros2 launch auto_bringup control.launch.py";
    rover.camera = "ros2 launch auto_bringup camera.launch.py";
    rover.software = "ros2 launch auto_bringup software.launch.py";
    mast.gps = "ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";
  };
in

# note i followed the running the auto stack guide which im pretty sure is now out of date due to drive changes LOL
# https://www.notion.so/Running-the-Auto-Stack-234b71396171801eb667cbc884e3b13b
# assumes ${dir} is consistent across all 3 platforms...
# this also prints the command that was run once it stops so it can manually be rerun if it errors or needs modification
pkgs.mkShell {
  shellHook = ''
    echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
    echo -e "Launching ${ansi.light-green}Auto${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover}${ansi.nc} and mast at ${ansi.light-purple}${mast}${ansi.nc}... Running in ${ansi.orange}${dir}${ansi.nc}"
    ssh-copy-id ${rover}
    ssh-copy-id ${mast}
    ${base-terminal "Base:Teleop" cmds.base.teleop} \
    & ${base-terminal "Base:Rviz" cmds.base.rviz} \
    & ${rover-terminal "Rover:GPS" cmds.rover.gps} \
    & ${rover-terminal "Rover:Control" cmds.rover.control} \
    & ${rover-terminal "Rover:Camera" cmds.rover.camera} \
    & ${rover-terminal "Rover:Software" cmds.rover.software} \
    & ptyxis --tab --title="Mast:GPS" -x "bash -c 'ssh -t ${mast} \"${dir}/${cmds.mast.gps}; echo -e Command: ${ansi.yellow}${dir}/${cmds.mast.gps}${ansi.nc}; exec bash -l\"; exec bash'"
    exit 0
  '';
}