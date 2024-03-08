import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",
  [RosService.SET_NIR_PROBE_LED]: "core/srv/SetNIRProbeLED",
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",
};
