import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",

  // Arm Related
  [RosService.READ_RFID]: "std_srvs/srv/Trigger",
  [RosService.START_AUTO_TYPING]: "arm_interfaces/srv/TypeSequence",
  [RosService.STOP_AUTO_TYPING]: "std_srvs/srv/Trigger",

  // Camera Related
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",

  // Error Related
  [RosService.BLCMD_RESET]: "blcmd_interfaces/srv/BLCMDReset",

  // Science Related
  [RosService.MIXERS]: "std_srvs/srv/SetBool",
  [RosService.KILN_COMMAND]: "science_interfaces/srv/KilnCommand",
  [RosService.SCIMBAL_COMMAND]: 'science_interfaces/srv/MoveScimbalCam',
  [RosService.HYDRAPROBE_COMMAND]: 'science_interfaces/srv/MoveHydraprobe',
  [RosService.TAKE_NIR_PROBE_READING]: "science_interfaces/srv/TakeNIRProbeReading",
  [RosService.MOVE_MICROSCOPE_SERVO]: "science_interfaces/srv/MoveMicroscopeServo",
  [RosService.THETA_360_CAM_CAPTURE]: "std_srvs/srv/Trigger",
  [RosService.CALL_RAMAN_SPEC]: "science_interfaces/srv/RamanSpec",
  [RosService.CALL_RAMAN_MECH]: "science_interfaces/srv/RamanMech",
  [RosService.UV_VIS_LED_1]: "std_srvs/srv/SetBool",
  [RosService.UV_VIS_LED_2]: "std_srvs/srv/SetBool",
  [RosService.CACHE_1]: "science_interfaces/srv/CacheCommand",
  [RosService.CACHE_2]: "science_interfaces/srv/CacheCommand",
  [RosService.HEATER]: "science_interfaces/srv/KilnCommand",
  [RosService.REQUEST_HYDRAPROBE_READING]: "std_srvs/srv/Trigger",
  [RosService.CAROUSEL]: "science_interfaces/srv/KilnCommand",
  [RosService.RGBInput]: "nova_interfaces/srv/RGBInput",

  // Autonomous Related
  [RosService.CARTOGRAPHER_COMMAND]: "nova_interfaces/srv/CartographerCommand",
};
