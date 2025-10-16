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
      {name = "Base:Teleop"; platform=base; cmd="ros2 launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:Drive"; platform=rover; cmd="ros2 launch drive_bringup drive.launch.py";}
    ];
  };
  old-drive-setup = {
    pre = pre-shell {payload-name="Old Drive"; need-rover=true;};
    terminals = [
      {name = "Base:Base"; platform=base; cmd="ros2 launch nova_bringup base.launch.py";}
      {name = "Rover:Old Drive"; platform=rover; cmd="ros2 launch nova_bringup old_drive.launch.py";}
    ];
  };
in
{
  # by default, use current drive
  drive = bashBuilder drive-setup "run-drive";
  old-drive = bashBuilder old-drive-setup "run-old-drive";
}
