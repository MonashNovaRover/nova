{ lib
, buildROSWorkspace
, buildEnv
, rviz2
, ros-gz
, gz-ros2-control
, rqt
, rqt-common-plugins
, gdb
, pythonPackages
, gps-umd
, tf2-tools
, reolink
, moveit-core
, moveit-kinematics
, ublox-dgnss
, teleop-modular
, teleop-modular-core
, teleop-modular-twist
, teleop-modular-control-mode
, teleop-modular-input-source
, teleop-modular-node
, teleop-modular-python-utils
, livox-ros-driver2

, nova-electronics ? throw "electronics is needed, but not available!"
, nova-science ? throw "science is needed, but not available!"
, nova-cameras2 ? throw "cameras2 is needed, but not available!"
, nova-gui ? throw "gui is needed, but not available!"
, nova-drive ? throw "drive is needed, but not available!"
, nova-drive-interfaces ? throw "drive-interfaces is needed, but not available!"
, nova-blcmd-interfaces ? throw "blcmd-interfaces is needed, but not available!"
, nova-blcmd-utils ? throw "blcmd-utils is needed, but not available!"
, nova-arm-interfaces ? throw "arm-interfaces is needed, but not available!"
, nova-arm ? throw "arm is needed, but not available!"
, nova-input-interfaces ? throw "input-interfaces is needed, but not available!"
, nova-inputs ? throw "inputs is needed, but not available!"
, nova-cmd-interfaces ? throw "cmd-interfaces is needed, but not available!"
, nova-cmd-utils ? throw "cmd-utils is needed, but not available!"
, nova-gimbal-cam ? throw "gimbal-cam is needed, but not available!"
, nova-interfaces ? throw "nova-interfaces is needed, but not available!"
, nova-bringup ? throw "nova-bringup is needed, but not available!"
, nova-auto-bringup ? throw "auto-bringup is needed, but not available!"
, nova-arm-bringup ? throw "arm-bringup is needed, but not available!"
, nova-drive-bringup ? throw "drive-bringup is needed, but not available!"
, nova-rover-description ? throw "rover-description is needed, but not available!"
, nova-blcmd-hardware ? throw "nova-blcmd-hardware is needed, but not available!"
, nova-blcmd-hardware2 ? throw "nova-blcmd-hardware2 is needed, but not available!"
, nova-cmd-hardware ? throw "nova-cmd-hardware is needed, but not available!"
, nova-controller-common ? throw "nova-controller-common is needed, but not available!"
, nova-drive-controller-base ? throw "nova-drive-controller-base is needed, but not available!"
, nova-pivot-drive-controller ? throw "nova-pivot-drive-controller is needed, but not available!"
, nova-strafe-drive-controller ? throw "nova-strafe-drive-controller is needed, but not available!"
, nova-diff-drive-controller ? throw "nova-diff-drive-controller is needed, but not available!"
, nova-teleop-drive-joy ? throw "nova-teleop-drive-joy is needed, but not available!"
, nova-teleop-arm-joy ? throw "nova-teleop-arm-joy is needed, but not available!"
, nova-gazebo ? throw "nova-gazebo is needed, but not available!"
, nova-python-control ? throw "python-control is needed, but not available!"
, nova-python-control2 ? throw "python-control2 is needed, but not available!"
, nova-excavation-construction ? throw "excavation-construction is needed, but not available!"
, nova-utils ? throw "nova-utils is needed, but not available!"
, nova-arm-controller ? throw "nova-arm-controller is needed, but not available!"
, nova-twistmapper ? throw "nova-twistmapper is needed, but not available!"
, nova-path-planner ? throw "nova-path-planner is needed, but not available!"
, nova-banksia-kinematics-plugin ? throw "nova-banksia-kinematics-plugin is needed, but not available!"
, nova-waratah-kinematics-plugin ? throw "nova-waratah-kinematics-plugin is needed, but not available!"
, gpsd-client ? throw "gpsd-client is needed, but not available!"
, nova-joint-space-control-mode ? throw "nova-joint-space-control-mode is need, but not available!"
, nova-teleop-arm ? throw "nova-teleop-arm is need, but not available!"
, launchPackages

  # Configuration options
  ## Include graphical applications in the workspace.
, graphical ? true

  ## Configure the workspace for interactive use.
, interactive ? true

  ## Manually specify which Nova Rover packages to include.
  ## Note that some packages may have dependencies on others that will be
  ## implicitly included.
, novaPackages ? {
    inherit
      nova-electronics
      nova-science
      nova-cameras2
      nova-blcmd-hardware
      nova-blcmd-hardware2
      nova-cmd-hardware
      nova-controller-common
      nova-drive-controller-base
      nova-pivot-drive-controller
      nova-strafe-drive-controller
      nova-diff-drive-controller
      nova-teleop-drive-joy
      nova-teleop-arm-joy
      nova-gui
      nova-drive
      nova-drive-interfaces
      nova-blcmd-interfaces
      nova-blcmd-utils
      nova-arm-interfaces
      nova-arm
      nova-input-interfaces
      nova-inputs
      nova-cmd-interfaces
      nova-cmd-utils
      nova-gimbal-cam
      nova-interfaces
      nova-bringup
      nova-auto-bringup
      nova-arm-bringup
      nova-drive-bringup
      nova-rover-description
      nova-gazebo
      nova-python-control
      nova-python-control2
      nova-excavation-construction
      nova-utils
      nova-arm-controller
      nova-twistmapper
      nova-path-planner
      nova-banksia-kinematics-plugin
      nova-waratah-kinematics-plugin
      reolink
      nova-joint-space-control-mode
      nova-teleop-arm
      ;
    nova-launch-auto-arch = launchPackages.auto-arch;
    nova-launch-auto-urc = launchPackages.auto-urc;
    nova-launch-gui = launchPackages.gui;
    nova-launch-drive = launchPackages.drive;
    nova-launch-old-drive = launchPackages.old-drive;
  } 

  ## Extra packages to add to the workspace.
, extraPackages ? { 
    inherit
      tf2-tools
      moveit-core   # needed to dynamically load the kinematics_solver plugin for nova_twistmapper
      moveit-kinematics
      gpsd-client
      teleop-modular
      teleop-modular-core
      teleop-modular-twist
      teleop-modular-control-mode
      teleop-modular-input-source
      teleop-modular-node
      teleop-modular-python-utils
      livox-ros-driver2
      ;
}
}:

let
  buildWorkspace = buildROSWorkspace.override {
    buildROSWorkspace = buildWorkspace;
    buildROSEnv = args: buildEnv (args // {
      # There are too many packages to completely avoid collisions.
      # Warnings during build time should be carefully observed.
      ignoreCollisions = true;
    });
  };
in
(buildWorkspace {
  name = "nova";
  inherit interactive;
  devPackages = novaPackages;
  prebuiltPackages = (lib.optionalAttrs graphical {
    inherit
      rviz2
      ros-gz
      gz-ros2-control
      gps-umd
      rqt rqt-common-plugins
      ublox-dgnss;
  }) // extraPackages;
  prebuiltShellPackages = {
    inherit
      gdb;
  };
}).overrideAttrs ({ passthru ? { }, ... }: {
  passthru = passthru // {
    inherit novaPackages extraPackages;
  };
})
