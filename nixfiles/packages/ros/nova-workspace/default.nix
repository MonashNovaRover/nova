{ lib
, pkgs
, git-metadata
, buildROSWorkspace
, buildEnv
  # Configuration options
  ## Include graphical applications in the workspace.
, graphical ? true

  ## Configure the workspace for interactive use.
, interactive ? true

  ## Manually specify which Nova Rover packages to include.
  ## Note that some packages may have dependencies on others that will be
  ## implicitly included.
, novaPackages ? {
    inherit (pkgs.ros)
      #nova-dgnss
      nova-electronics
      nova-science
      nova-blcmd-hardware
      nova-blcmd-hardware2
      nova-cmd-hardware
      nova-qcmd-hardware
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
      nova-teleop-ec
      nova-utils
      nova-arm-controller
      nova-twistmapper
      nova-path-planner
      nova-banksia-kinematics-plugin
      nova-waratah-kinematics-plugin
      nova-joint-space-control-mode
      nova-teleop-arm
      nova-legacy-input-mode
      nova-teleop-science
      nova-science-interfaces
      nova-science-bringup
      nova-arm-kinematics
      nova-cameras
      nova-camera-msgs
      nova-locked-publisher
      ;
    inherit (pkgs)
      nova-launch-scripts
      reolink
      ;
    inherit (pkgs.python3Packages)
      nova-can-sleuth
      ;
    nova-git-metadata = (pkgs.writeTextDir "nova-git-metadata" git-metadata);
  }

  ## Extra packages to add to the workspace.
, extraPackages ? { 
    inherit (pkgs.ros)
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
      rmw-cyclonedds-cpp
      rqt-controller-manager
      ;
    inherit (pkgs)
      mbtileserver
      ;
    inherit (pkgs.python3Packages)
      ros2-unbag
      ;
}
}:

let
  buildWorkspace = buildROSWorkspace.override {
    buildROSWorkspace = buildWorkspace;
    buildROSEnv = args: (buildEnv (args // {
      # There are too many packages to completely avoid collisions.
      # Warnings during build time should be carefully observed.
      ignoreCollisions = true;
    })).overrideAttrs (_: {
      dontFixup = true;
    });
  };
in
(buildWorkspace {
  name = "nova";
  inherit interactive;
  devPackages = novaPackages;
  prebuiltPackages = (lib.optionalAttrs graphical {
    inherit (pkgs.ros)
      rviz2
      ros-gz
      gz-ros2-control
      gps-umd
      rqt rqt-common-plugins
      ublox-dgnss;
    inherit (pkgs)
      mission-planner
      mavproxy
      ;
  }) // extraPackages;
  prebuiltShellPackages = {
    inherit (pkgs)
      gdb;
  };
}).overrideAttrs ({ passthru ? { }, ... }: {
  passthru = passthru // {
    inherit novaPackages extraPackages;
  };
})
