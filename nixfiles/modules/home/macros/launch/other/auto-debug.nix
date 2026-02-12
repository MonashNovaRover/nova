# This nix-shell is designed to run on the base station to quickly spin up the entire auto stack!
{ 
    base,
    rover,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  name = "run-auto-debug";
  one = {
    pre = pre-shell {payload-name=name + " one"; need-rover=true;};
    terminals = [
      {name = "Base:Rviz"; platform=base; cmd="./ros2 launch auto_bringup rviz.launch.py";}
      {name = "Rover:Cameras"; platform=rover; cmd="./ros2 launch auto_bringup camera.launch.py";}
      # we need to add a ros node that listens for the camera topics before exiting, 
      # i know chetan made something like this so that we can chain it into rtabmap launch
      # double note: doesn't have to be on the same device and we will use good ol sleep commands for now
      {name = "Rover:RTABMAP"; platform=rover; cmd="echo wait for camera launch; sleep 5; ./ros2 launch auto_bringup ros2 launch auto_bringup rtabmap.launch.py";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=name + " two"; need-rover=true; };
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="./ros2 launch teleop_drive_joy teleop.launch.py";} # this is just in case operator needs to take over
      {name = "Rover:Drive"; platform=rover; cmd="./ros2 launch drive_bringup drive.launch.py auto:=True";}
      {name = "Rover:Lidar"; platform=rover; cmd="./ros2 launch auto_bringup lidar.launch.py";}
      {name = "Rover:Localization"; platform=rover; cmd="./ros2 launch auto_bringup localization.launch.py";}
      {name = "Rover:Nav2"; platform=rover; cmd="./ros2 launch auto_bringup navigation.launch.py";}
    ];
    post = post-shell;
  };

  combined = {
    pre = pre-shell {payload-name=name+" combined"; need-rover=true; };
    terminals = one.terminals ++ two.terminals;
    post = post-shell;
  };
in
{
  auto-debug-one = bashBuilder one (name+"-one");
  auto-debug-two = bashBuilder two (name+"-two");
  auto-debug-combined = bashBuilder combined (name+"-combined");
}