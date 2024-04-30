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
  [RosTopic.DRIVE_INFO]: "core/msg/DriveInfo",
  [RosTopic.DRIVE_TELEMETRY]: "core/msg/Telemetry",

  // Arm Related
  [RosTopic.ARM_TELEMETRY]: "core/msg/CMDsFeedback",
  [RosTopic.RFID_DATA]: "std_msgs/msg/String",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: "core/msg/BLCMDStatusArray",
  
  // Science Related
  [RosTopic.TOF]: "sensor_msgs/msg/Range",
  [RosTopic.KILN_DATA]: "core/msg/KilnData",
  [RosTopic.NIR_DATA]: "core/msg/NIRProbeData",
  [RosTopic.MICROSCOPE_SERVO]: "core/msg/MicroscopeServoInfo",
  [RosTopic.HYDRAPROBE_DATA]: "core/msg/HydraprobeData",
};
