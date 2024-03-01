import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",
  [RosService.READ_RFID]: "std_srvs/srv/Trigger"
};
