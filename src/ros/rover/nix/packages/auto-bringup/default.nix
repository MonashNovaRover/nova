{
  lib,
  pkgs,
  buildRosPackage,
  ament-cmake,
  launch,
  launch-ros,
  xacro,
  aruco-opencv,
  aruco-opencv-msgs,
  robot-state-publisher,
  controller-manager,
  ros2-control,
  ros-gz,
  ros2-controllers,
  pluginlib,
  robot-localization,
  image-view,
  navigation2,
  depthai-ros,
  rtabmap-ros,
  nova-behavior-tree,
  nova-costmap-2d,
  nova-pointcloud-filter,
  nova-rover-description,
  nova-gazebo,
  nova-auto-interfaces,
  nova-bt-navigators,
  rviz-imu-plugin,
  imu-transformer,
  nova-pivot-drive-controller,
  tf2-tools,
  yolo-ros,
  lattice-primitive-generator,
}:

buildRosPackage rec {
  name = "auto-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "auto-bringup-source";
    path = ../../../auto_bringup;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [
    launch
    launch-ros
  ];

  passthru.workspacePackages = {
    inherit
      xacro
      robot-state-publisher
      controller-manager
      ros2-control
      ros-gz
      ros2-controllers
      aruco-opencv
      aruco-opencv-msgs
      pluginlib
      robot-localization
      image-view
      navigation2
      depthai-ros
      rtabmap-ros
      nova-behavior-tree
      nova-costmap-2d
      nova-pointcloud-filter
      nova-rover-description
      nova-gazebo
      nova-auto-interfaces
      nova-bt-navigators
      rviz-imu-plugin
      nova-pivot-drive-controller
      tf2-tools
      imu-transformer
      yolo-ros
      lattice-primitive-generator;
  };

  # After installing params and resources folders in nix store's auto_bringup,
  # we need to generate absolute filepaths for files in that auto_bringup, to
  # point to the nix store's folders
  buildInputs = [
    pkgs.jq
    pkgs.yq
  ];
  postInstall = ''
    # Generate absolute nix store filepaths for JSON files
    jsonFilepath="$out/share/auto_bringup/resources/YOLOv11/best.json"
    jsonFile=$(cat $jsonFilepath)

    updatedJsonFile=$(echo "$jsonFile" | jq --arg out "$out" '. + {
      model: {
        bin: "\($out)/share/auto_bringup/resources/YOLOv11/best.bin",
        model_name: "\($out)/share/auto_bringup/resources/YOLOv11/best_openvino_2022.1_6shave.blob",
        xml: "\($out)/share/auto_bringup/resources/YOLOv11/best.xml",
        zoo: "path"
      }
    }')

    echo "$updatedJsonFile" > $jsonFilepath

    # Generate absolute nix store filepaths for YAML files
    yamlFilepath="$out/share/auto_bringup/params/oak.yaml"

    yq -y -i "
      .\"/oak\".ros__parameters.nn.i_nn_config_path = \"$jsonFilepath\"
    " $yamlFilepath
  '';
}
