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
  # these run a single command, and record it in history, you will need to make a custom line for multiple commands
  base-terminal = name: cmd: ''ptyxis --tab --title="${name}" -x "bash -ic '${cmd}; history -s ${cmd}; exec bash'"'';
  rover-terminal = name: cmd: ''ptyxis --tab --title="${name}" -x "bash -c 'ssh -t ${rover} \"bash -ic \\\"${cmd}; history -s ${cmd}; exec bash -l\\\"\"; exec bash'"'';
  mast-terminal = name: cmd: ''ptyxis --tab --title="${name}" -x "bash -c 'ssh -t ${mast} \"bash -ic \\\"${cmd}; history -s ${cmd}; exec bash -l\\\"\"; exec bash'"'';

  cmds = {
    base.teleop = "launch-teleop";
    base.rviz = "launch-rviz";
    rover.gps = "launch-gps";
    rover.control = "launch-control";
    rover.camera = "launch-oaks";
    rover.software = "launch-auto-software";
    mast.gps = "ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";
  };
in

# note i followed the running the (URC) auto stack guide which im pretty sure is now out of date
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
    & ${mast-terminal "Mast:GPS" cmds.mast.gps}
    exit 0
  '';
}