self: super:

{
  ros = super.ros.overrideScope (rosSelf: rosSuper: {
    # Custom library functions again (the ROS overlay does not use the existing lib overlay properly).
    lib = rosSuper.lib // (import ../../lib self rosSelf.lib);
  });
}
