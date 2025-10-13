# simple drive with rover and xbox controller with base station
# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
# by default it will run with current drive, but old drive can be specified with -A drive.old
{ 
    base,
    rover,
    pre-shell,
    mkBashScript,
}:

let 
  drive-setup = {
    pre = pre-shell {payload-name="Drive"; need-rover=true;};
    terminals = [
      {name = "Base:Teleop"; platform=base; cmd="launch-teleop";}
      {name = "Rover:Drive"; platform=rover; cmd="launch-drive";}
    ];
  };
  old-drive-setup = {
    pre = pre-shell {payload-name="Old Drive"; needrover=true;};
    terminals = [
      {name = "Base:Base"; platform=base; cmd="launch-base";}
      {name = "Rover:Old Drive"; platform=rover; cmd="launch-old-drive";}
    ];
  };
in
{
  # by default, use current drive
  default = mkBashScript drive-setup;
  old = mkBashScript old-drive-setup;
}
