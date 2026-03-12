{ 
    base,
    base-nix,
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
      {name = "Base:Rviz"; platform=base; cmd="./rviz2 -d ../share/arm_bringup/rviz/arm.rviz | grep -v TF_NAN";}
      {name = "Rover:Reolink Ctl"; platform=rover; cmd="./reolink-ctl";}
      {name="Base:Reolink"; platform=base-nix "nix-shell -p ffmpeg"; cmd="reolink low";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; };
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="ros2 launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:Drive"; platform=rover; cmd="launch-drive";}
      {name = "Rover:GUI"; platform=base; cmd="gui-shell --command \"gui-run\"";}
      {name = "Rover:GUI Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = post-shell;
  };

  mock = {
    pre = pre-shell {payload-name=task-name + " mock"; need-rover=false;};
    terminals = [
      {name = "Rover:Arm Control"; platform=base; cmd="./ros2 launch arm_bringup mock.launch.py local:=True | grep -v not.defined.in";}
      {name = "Base:Arm Teleop"; platform=base; cmd="./ros2 launch teleop_arm teleop.launch.py local:=True log_inputs:=True";}
      {name = "Base:Rviz"; platform=base; cmd="./rviz2 -d ../share/arm_bringup/rviz/arm.rviz | grep -v TF_NAN";}
    ];
    post = post-shell;
  };

  mock-controller = {
    pre = pre-shell {payload-name=task-name + " mock with controller"; need-rover=false;};
    terminals = [
      {name = "Rover:Arm Control"; platform=base; cmd="./ros2 launch arm_bringup mock.launch.py local:=True | grep -v not.defined.in";}
      {name = "Base:Arm Teleop"; platform=base; cmd="./ros2 launch teleop_arm teleop.launch.py local:=True log_inputs:=True joysticks:=false";}
      {name = "Base:Rviz"; platform=base; cmd="./rviz2 -d ../share/arm_bringup/rviz/arm.rviz | grep -v TF_NAN";}
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
  pl-mock = bashBuilder mock (task-name+"-mock");
  pl-mock-controller = bashBuilder mock-controller (task-name+"-mock-controller");
  pl-combined = bashBuilder combined (task-name+"-combined");
}
