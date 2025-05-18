import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",

  // Arm Related
  [RosService.READ_RFID]: "std_srvs/srv/Trigger",

  // Camera Related
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",

  // Error Related
  [RosService.BLCMD_RESET]: "blcmd_interfaces/srv/BLCMDReset",

  // Science Related
  [RosService.MIXERS]: "std_srvs/srv/SetBool",
  [RosService.KILN_COMMAND]: "nova_interfaces/srv/KilnCommand",
  [RosService.SCIMBAL_COMMAND]: 'nova_interfaces/srv/MoveScimbalCam',
  [RosService.HYDRAPROBE_COMMAND]: 'nova_interfaces/srv/MoveHydraprobe',
  [RosService.TAKE_NIR_PROBE_READING]: "nova_interfaces/srv/TakeNIRProbeReading",
  [RosService.MOVE_MICROSCOPE_SERVO]: "nova_interfaces/srv/MoveMicroscopeServo",
  [RosService.THETA_360_CAM_CAPTURE]: "std_srvs/srv/Trigger",
  [RosService.CALL_RAMAN_SPEC]: "nova_interfaces/srv/RamanSpec",
  [RosService.CALL_RAMAN_MECH]: "nova_interfaces/srv/RamanMech",
  [RosService.UV_VIS_LED_1]: "std_srvs/srv/SetBool",
  [RosService.UV_VIS_LED_2]: "std_srvs/srv/SetBool",
  [RosService.CACHE_1]: "nova_interfaces/srv/CacheCommand",
  [RosService.CACHE_2]: "nova_interfaces/srv/CacheCommand",
  [RosService.HEATER]: "std_srvs/srv/SetBool",
};
