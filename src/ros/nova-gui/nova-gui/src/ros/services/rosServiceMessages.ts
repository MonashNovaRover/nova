import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",

  // Arm Related
  [RosService.READ_RFID]: "std_srvs/srv/Trigger",

  // Camera Related
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",

  // Error Related
  [RosService.BLCMD_RESET]: "blcmd_interfaces/srv/BLCMDReset",

  // Science Related
  [RosService.KILN_COMMAND]: "nova_interfaces/srv/KilnCommand",
  [RosService.SET_NIR_PROBE_LED]: "nova_interfaces/srv/SetNIRProbeLED",
  [RosService.MOVE_MICROSCOPE_SERVO]: "nova_interfaces/srv/MoveMicroscopeServo",
  [RosService.THETA_360_CAM_CAPTURE]: "std_srvs/srv/Trigger",
};
