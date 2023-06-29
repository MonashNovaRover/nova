repos:

{
  rosPackages = {
    core = repos.rover + /core;
    control = repos.rover + /control;
    autonomous = repos.rover + /autonomous;
    electronics = repos.rover + /electronics;
    science = repos.rover + /science;
    camera-msgs = repos.cameras2 + /camera_msgs;
    cameras2 = repos.cameras2 + /cameras2;
  };

  pythonPackages = {
    coms-utils = repos.coms-utils;
  };
}
