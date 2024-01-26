import { RosTopics } from "./rosTopics";

/**
 * This Object maps ROS Topics found in `rosTopics.ts` to Plain ROS Message types
 * Not to be confused with `rosMessageTypes.ts`
 *
 * The Topic RosTopics.DEMO_TOPIC is being mapped to "custom_msgs/demo_msg"
 *
 * "custom_msgs/demo_msgs" comes purely from ROS
 */
export const rosMessages = {
  [RosTopics.NULL_TOPIC]: "",
  [RosTopics.POSE]: "geometry_msgs/msg/Pose",
  [RosTopics.DRIVE_INFO]: "core/msg/DriveInfo",
  [RosTopics.TELEMETRY]: "core/msg/Telemetry"
};
