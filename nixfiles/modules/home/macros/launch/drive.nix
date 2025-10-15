# simple drive with rover and controller with base station
# by default it will run with current drive, but old drive can be specified with -A drive.old
{ 
    base,
    rover,
    pre-shell,
    bashBuilder,
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
    pre = pre-shell {payload-name="Old Drive"; need-rover=true;};
    terminals = [
      {name = "Base:Base"; platform=base; cmd="launch-base";}
      {name = "Rover:Old Drive"; platform=rover; cmd="launch-old-drive";}
    ];
  };
in
{
  # by default, use current drive
  default = bashBuilder drive-setup "run-drive";
  old = bashBuilder old-drive-setup "run-old-drive";
}
