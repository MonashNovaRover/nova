{ callPackage, pkgs }:
{
  audio-msgs = callPackage ./audio-msgs { };
  depthai = callPackage ./depthai { };
  direct-visual-lidar-calibration = callPackage ./direct-visual-lidar-calibration { };
  fast-calib = callPackage ./fast-calib { };
  fast-livo2 = callPackage ./fast-livo2 { };
  ground-segmentation = callPackage ./ground-segmentation { };
  ground-segmentation-ros2 = callPackage ./ground-segmentation-ros2 { };
  gst-bridge = callPackage ./gst-bridge { };
  gst-msgs = callPackage ./gst-msgs { };
  gst-pipeline = callPackage ./gst-pipeline { };
  gst-pipeline-plugins = callPackage ./gst-pipeline-plugins { };
  gst-pipeline-plugins-webrtc = callPackage ./gst-pipeline-plugins-webrtc { };
  iridescence = callPackage ./iridescence { };
  livox-ros-driver2 = callPackage ./livox-ros-driver2 { };
  livox-sdk2 = callPackage ./livox-sdk2 { };
  nova-workspace = callPackage ./nova-workspace { };
  nova-workspace-mast = callPackage ./nova-workspace {
    graphical = false;
    novaPackages = {
      nova-electronics = pkgs.nova-electronics;
      nova-bringup = pkgs.nova-bringup;
    };
    extraPackages = {};
  };
  rclnodejs = callPackage ./rclnodejs { };
  ros-tcp-endpoint = callPackage ./ros-tcp-endpoint { };
  ros-typescript-definitions = callPackage ./ros-typescript-definitions { };
  spatio-temporal-voxel-layer = callPackage ./spatio-temporal-voxel-layer { };
  vikit-common = callPackage ./vikit-common { };
  vikit-ros = callPackage ./vikit-ros { };
  yolo-ros = callPackage ./yolo-ros { };
  yolo-msgs = callPackage ./yolo-msgs { };
  yolo-bringup = callPackage ./yolo-bringup { };
}
