{ 
    base,
    rover,
    pi5,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  task-name = "ma";
  
  # everything that is run on the rover and pi5
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true; need-pi5=true;};
    terminals = [
      {name = "Rover:Drive"; platform=rover; cmd="./ros2 launch drive_bringup drive.launch.py auto:=true";}
      {name = "Rover:Navigation"; platform=rover; cmd="./ros2 launch auto_bringup navigation.launch.py";}
      {name = "Pi5:Realsense"; platform=pi5; cmd="./ros2 launch auto_bringup realsense.launch.py";}
      {name = "Pi5:LidarMapping"; platform=pi5; cmd="./ros2 launch auto_bringup lidar.launch.py";}
    ];
    post = post-shell;
  };

  # everything that is run on base
  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; need-pi5=true;};
    terminals = [
      {name = "Base:Rviz"; platform=base; cmd="./ros2 launch auto_bringup rviz.launch.py";}
      {name = "Base:Teleop"; platform=base; cmd="./ros2 launch drive_bringup teleop.launch.py";} # this is just in case operator needs to take over
      # @Felicity to fill out for gui cameras
      {name = "Rover:GUI"; platform=base; cmd="gui-shell --command \"gui-run\"";}
      {name = "Rover:GUI Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = post-shell;
  };

  combined = {
    pre = pre-shell {payload-name=task-name+" combined"; need-rover=true; need-pi5=true;};
    terminals = one.terminals ++ two.terminals;
    post = post-shell;
  };
in
{
  ma-one = bashBuilder one (task-name+"-one");
  ma-two = bashBuilder two (task-name+"-two");
  ma-combined = bashBuilder combined (task-name+"-combined");
}