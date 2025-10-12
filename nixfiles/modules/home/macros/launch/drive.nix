# simple drive with rover and xbox controller with base station
# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    pkgs,
    ansi,
    rover,
    base-terminal,
    rover-terminal,
}:

let 
  cmds = {
    base.teleop = "launch-teleop";
    rover.drive = "launch-drive";

    base.old-base = "launch-base";
    rover.old-drive = "launch-old-drive";
  };
in

# note i followed the running the (URC) auto stack guide which im pretty sure is now out of date
# havent put in the appropriate arguments for arch and urc respectively
# https://www.notion.so/Running-the-Auto-Stack-234b71396171801eb667cbc884e3b13b
{
  # by default, use current drive
  default = pkgs.mkShell {
    shellHook = ''
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}Drive${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover}${ansi.nc}"
      ssh-copy-id ${rover}
      ${base-terminal "Base:Teleop" cmds.base.teleop} \
      & ${rover-terminal "Rover:Drive" cmds.rover.drive}
      exit 0
    '';
  };

  old = pkgs.mkShell {
    shellHook = ''
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}"
      echo -e "Launching ${ansi.light-green}Old Drive${ansi.nc}... SSHing into orin at ${ansi.light-purple}${rover}${ansi.nc}"
      ssh-copy-id ${rover}
      ${base-terminal "Base:Base" cmds.base.old-base} \
      & ${rover-terminal "Rover:Old Drive" cmds.rover.old-drive}
      exit 0
    '';
  };


}
