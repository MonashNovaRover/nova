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
  [RosTopic.ARM_TELEMETRY]: "core/msg/CMDsFeedback",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: "core/msg/BLCMDStatusArray",
  
  // Science Related
  [RosTopic.KILN_DATA]: "core/msg/KilnData",
  [RosTopic.NIR_DATA]: "core/msg/NIRProbeData",
};
