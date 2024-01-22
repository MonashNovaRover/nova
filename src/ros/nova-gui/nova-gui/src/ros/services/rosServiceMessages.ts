import { RosService } from "./rosServices";

/**
 * This Object maps ROS Topics found in `rosTopics.ts` to Plain ROS Message types
 * Not to be confused with `rosMessageTypes.ts`
 *
 * The Topic RosTopics.DEMO_TOPIC is being mapped to "custom_msgs/demo_msg"
 *
 * "custom_msgs/demo_msgs" comes purely from ROS
 */
export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",
  [RosService.START_STREAM]: "camera_msgs/CameraOperation",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",
};
