{ 
    base,
    rover,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  task-name = "pl";
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true;};
    terminals = [
      {name = "Rover:Arm Control"; platform=rover; cmd="./ros2 launch arm_bringup control.launch.py local:=True | grep -v not.defined.in";}
      {name = "Rover:Arm Can Sleuth"; platform=rover; cmd="./can_sleuth -o tui taipan";}
      {name = "Base:Arm Teleop"; platform=base; cmd="./ros2 launch teleop_arm teleop.launch.py local:=True log_inputs:=True";}
      {name = "Base:Rviz"; platform=base; cmd="./rviz2 ../share/arm-bringup/rviz/arm.rviz";}
      {name = "Rover:Reolink Ctl"; platform=rover; cmd="./reolink-ctl";}
      # TODO: this needs ffmpeg so it dont work
      {name = "Rover:Reolink"; platform=base; cmd="reolink low";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; };
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="ros2 launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:Drive"; platform=rover; cmd="launch-drive";}
    ];
    post = post-shell;
  };

  combined = {
    pre = pre-shell {payload-name=task-name+" combined"; need-rover=true; };
    terminals = one.terminals ++ two.terminals;
    post = post-shell;
  };
in
{
  pl-one = bashBuilder one (task-name+"-one");
  pl-two = bashBuilder two (task-name+"-two");
  pl-combined = bashBuilder combined (task-name+"-combined");
}