{ 
    base,
    rover,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  task-name = "an";
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true;};
    terminals = [
      {name = "Base:Rviz"; platform=base; cmd="ros2 launch auto_bringup rviz.launch.py";}
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
  an-one = bashBuilder one ("run-"+task-name+"-one");
  an-two = bashBuilder two ("run-"+task-name+"-two");
  an-combined = bashBuilder combined ("run-"+task-name+"-combined");
}