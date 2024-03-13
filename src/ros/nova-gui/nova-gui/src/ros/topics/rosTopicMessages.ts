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
  [RosTopic.TELEMETRY]: "core/msg/Telemetry",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Science Related
  [RosTopic.TOF]: "sensor_msgs/msg/Range"
};
