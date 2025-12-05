{ 
    base,
    rover,
    mast,
    pre-shell,
    post-shell,
    bashBuilder
}:

let 
  task-name = "an"; # we might need to add delays to some of these commands?
  one = {
    pre = pre-shell {payload-name=task-name + " one"; need-rover=true;};
    terminals = [
      {name="Base:Rviz"; platform=base; cmd="ros2 launch auto_bringup rviz.launch.py";}
      {name="Rover:Software"; platform=rover; cmd="ros2 launch auto_bringup software.launch.py";}
    ];
    post = post-shell;
  };

  two = {
    pre = pre-shell {payload-name=task-name + " two"; need-rover=true; };
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="ros2 launch teleop_drive_joy teleop.launch.py";}
      {name="Rover:Drive"; platform=rover; cmd="ros2 launch drive_bringup drive.launch.py auto:=True";}
      {name="Rover:Cameras"; platform=rover; cmd="ros2 launch auto_bringup camera.launch.py";}
      {name="Run:Gui+Maps"; platform=base; cmd="run-gui-maps";} # need to test if this works
    ];
    post = post-shell;
  };

  mast-setup = { # this won't be added to combined
      pre = pre-shell {payload-name=task-name + " mast"; need-mast=true; };
      terminals = [
        {name="Mast:GPS"; platform=mast; cmd="ros2 launch nova_bringup gps_base.launch.py gps_params:=/home/nova/gps.yaml";}
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
  an-one = bashBuilder one (task-name+"-one");
  an-two = bashBuilder two (task-name+"-two");
  an-combined = bashBuilder combined (task-name+"-combined");
  an-mast = bashBuilder mast-setup (task-name+"-mast");
}