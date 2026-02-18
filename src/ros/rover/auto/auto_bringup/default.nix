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
  image-transport,
  navigation2,
  depthai-ros,
  rtabmap-ros,
  nova-behavior-tree,
  nova-object-localisation,
  nova-costmap-2d,
  nova-rover-description,
  nova-gazebo,
  nova-auto-interfaces,
  nova-bt-navigators,
  nova-cameras2,
  rviz-imu-plugin,
  rviz-satellite,
  imu-transformer,
  nova-pivot-drive-controller,
  tf2-tools,
  rqt-tf-tree,
  yolo-ros,
  lattice-primitive-generator,
  spatio-temporal-voxel-layer,
  nova-interfaces,
  imu-filter-madgwick,
  realsense2-camera,
  usb-cam,
}:

buildRosPackage rec {
  name = "auto-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "auto-bringup-source";
    path = ./.;
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
      image-transport
      navigation2
      depthai-ros
      rtabmap-ros
      nova-behavior-tree
      nova-object-localisation
      nova-costmap-2d
      nova-rover-description
      nova-gazebo
      nova-auto-interfaces
      nova-bt-navigators
      nova-cameras2
      rviz-imu-plugin
      rviz-satellite
      nova-pivot-drive-controller
      tf2-tools
      rqt-tf-tree
      imu-transformer
      yolo-ros # this is only used in sim, so if space is needed on rover, comment out this package. (Used for nova-object-localisation)
      spatio-temporal-voxel-layer
      lattice-primitive-generator
      nova-interfaces
      imu-filter-madgwick
      realsense2-camera
      usb-cam
      ;
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
    jsonFilepath="$out/share/auto_bringup/resources/YOLO_URC_2025/yolo11s.json"
    jsonFile=$(cat $jsonFilepath)

    updatedJsonFile=$(echo "$jsonFile" | jq --arg out "$out" '. + {
      model: {
        bin: "\($out)/share/auto_bringup/resources/YOLO_URC_2025/yolo11s.bin",
        model_name: "\($out)/share/auto_bringup/resources/YOLO_URC_2025/yolo11s_openvino_2022.1_6shave.blob",
        xml: "\($out)/share/auto_bringup/resources/YOLO_URC_2025/yolo11s.xml",
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
