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
  [RosTopic.DRIVE_INFO]: "core/msg/DriveInfo",
  [RosTopic.TELEMETRY]: "core/msg/Telemetry",
  [RosTopic.KILN_TEMP]: "core/msg/KilnTempData",
};
