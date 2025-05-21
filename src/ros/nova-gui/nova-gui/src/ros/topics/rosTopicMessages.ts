import { RosTopic } from "./rosTopic";

/**
 * This Object maps ROS Topics found in `rosTopics.ts` to Plain ROS Message types
 * Not to be confused with `rosMessageTypes.ts`
 *
 * The Topic RosTopics.DEMO_TOPIC is being mapped to "custom_msgs/demo_msg"
 *
 * "custom_msgs/demo_msgs" comes purely from ROS
 */
export const rosTopicMessages = {
  [RosTopic.NULL_TOPIC]: "",
  [RosTopic.POSE]: "geometry_msgs/msg/Pose",

  // Drive Related
  [RosTopic.DRIVE_INFO]: "drive_interfaces/msg/DriveInfo",
  [RosTopic.DRIVE_TELEMETRY]: "blcmd_interfaces/msg/Telemetry",

  // Arm Related
  [RosTopic.ARM_TELEMETRY]: "cmd_interfaces/msg/CMDsFeedback",
  [RosTopic.RFID_DATA]: "std_msgs/msg/String",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: "blcmd_interfaces/msg/BLCMDStatusArray",

  // Science Related
  [RosTopic.TOF]: "sensor_msgs/msg/Range",
  [RosTopic.KILN_DATA]: "nova_interfaces/msg/KilnData",
  [RosTopic.NIR_DATA]: "nova_interfaces/msg/NIRProbeData",
  [RosTopic.MICROSCOPE_SERVO]: "nova_interfaces/msg/MicroscopeServoInfo",
  [RosTopic.UV_VIS_SPEC]: "nova_interfaces/msg/UVVisSpecData",
  [RosTopic.THETA_360_CAM_IMAGE]: "sensor_msgs/msg/CompressedImage",
  [RosTopic.HYDRAPROBE_DATA]: "nova_interfaces/msg/HydraprobeData",
  [RosTopic.RAMAN_SPEC_MSG]: "nova_interfaces/msg/RamanSpectrum",
  [RosTopic.RAMAN_MECH_MSG]: "nova_interfaces/msg/RamanState",
  [RosTopic.BME_SENSOR]: "nova_interfaces/msg/BMESensor",

  // Maps Related
  [RosTopic.ROVER_LOCATION]: "nova_interfaces/msg/RoverPoseGPS",
  [RosTopic.BASE_LOCATION]: "nova_interfaces/msg/RoverPoseGPS",

  // Other
  [RosTopic.BATTERY_STATE]: "sensor_msgs/msg/BatteryState",
  [RosTopic.ACTIVATED_NODES]: "nova_interfaces/msg/ActiveNodeStatus",
};
