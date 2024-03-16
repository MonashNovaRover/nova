import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",

  // Arm Related
  [RosService.READ_RFID]: "std_srvs/srv/Trigger",

  // Camera Related
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",

  // Science Related 
  [RosService.KILN_COMMAND]: "core/srv/KilnCommand",
  [RosService.SET_NIR_PROBE_LED]: "core/srv/SetNIRProbeLED",
  [RosService.MOVE_MICROSCOPE_SERVO]: "core/srv/MoveMicroscopeServo",
};
