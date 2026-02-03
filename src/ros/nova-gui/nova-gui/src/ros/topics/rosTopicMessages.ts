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
  [RosTopic.KEYBOARD_DATA]: "arm_interfaces/msg/KeyboardPoints",
  [RosTopic.TYPE_SEQUENCE]: "arm_interfaces/msg/SequencerFeedback",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: "blcmd_interfaces/msg/BLCMDStatusArray",

  // Science Related
  [RosTopic.TOF]: "sensor_msgs/msg/Range",
  [RosTopic.KILN_DATA]: "science_interfaces/msg/KilnData",
  [RosTopic.PELTIER_STATUS]: "science_interfaces/msg/EffortStatus",
  [RosTopic.DIAPHRAGM_PUMP_STATUS]: "science_interfaces/msg/EffortStatus",
  [RosTopic.NIR_DATA]: "science_interfaces/msg/NIRProbeData",
  [RosTopic.MICROSCOPE_SERVO]: "science_interfaces/msg/MicroscopeServoInfo",
  [RosTopic.UV_VIS_SPEC]: "science_interfaces/msg/UVVisSpecData",
  [RosTopic.THETA_360_CAM_IMAGE]: "sensor_msgs/msg/CompressedImage",
  [RosTopic.HYDRAPROBE_DATA]: "science_interfaces/msg/HydraprobeData",
  [RosTopic.RAMAN_SPEC_MSG]: "science_interfaces/msg/RamanSpectrum",
  [RosTopic.RAMAN_MECH_MSG]: "science_interfaces/msg/RamanState",
  [RosTopic.BME_SENSOR]: "science_interfaces/msg/BMESensor",
  [RosTopic.AUGER1_DEPTH_SENSOR]: "std_msgs/msg/Bool",
  [RosTopic.AUGER2_DEPTH_SENSOR]: "std_msgs/msg/Bool",

  // Maps Related
  [RosTopic.ROVER_LOCATION]: "sensor_msgs/msg/NavSatFix",
  [RosTopic.BASE_LOCATION]: "sensor_msgs/msg/NavSatFix",
  [RosTopic.AUTO_STATUS]: "nova_interfaces/msg/Status",

  // Other
  [RosTopic.BATTERY_STATE]: "sensor_msgs/msg/BatteryState",
  [RosTopic.ACTIVATED_NODES]: "nova_interfaces/msg/ActiveNodeStatus",
  [RosTopic.RADIO_STATUS]: "nova_interfaces/msg/RadioStatus",
};
