# simple drive with rover and xbox controller with base station
# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
# by default it will run with current drive, but old drive can be specified with -A drive.old
{ 
    base,
    rover,
    pre-shell,
    post-shell,
    make-shell,
}:

let 
  drive-setup = {
    pre-shell = pre-shell {platform-name="Drive";};
    terminals = [
      {name = "Base:Teleop"; platform=base; cmd="launch-teleop";}
      {name = "Rover:Drive"; platform=rover; cmd="launch-drive";}
    ];
    post-shell = post-shell;
  };
  old-drive-setup = {
    pre-shell = pre-shell {platform-name="Old Drive";};
    terminals = [
      {name = "Base:Base"; platform=base; cmd="launch-base";}
      {name = "Rover:Old Drive"; platform=rover; cmd="launch-old-drive";}
    ];
    post-shell = post-shell;
  };
in
{
  # by default, use current drive
  default = make-shell drive-setup;
  old = make-shell old-drive-setup;
}
