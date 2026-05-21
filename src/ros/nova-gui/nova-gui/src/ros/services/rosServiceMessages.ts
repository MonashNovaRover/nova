import { RosService } from "./rosService";

export const rosServiceMessages = {
  [RosService.NULL_SERVICE]: "",

  // Drive Related
  [RosService.TELEOP_DRIVE_SET_PARAMS]: "rcl_interfaces/srv/SetParameters",

  // Arm Related
  [RosService.READ_RFID]: "std_srvs/srv/Trigger",
  [RosService.START_AUTO_TYPING]: "arm_interfaces/srv/TypeSequence",
  [RosService.STOP_AUTO_TYPING]: "std_srvs/srv/Trigger",

  // Camera Related
  [RosService.START_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PAUSE_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.STOP_CAMS]: "camera_msgs/srv/CameraOperation",
  [RosService.PRESET_CAMS]: "camera_msgs/srv/CameraProfileSelection",
  [RosService.GET_IP_LIST]: "camera_msgs/srv/GetIPList",

  // Error Related
  [RosService.BLCMD_RESET]: "blcmd_interfaces/srv/BLCMDReset",

  // Science Related
  [RosService.THERMAL_COMMAND]: "science_interfaces/srv/ThermalCommand",
  [RosService.WATER_PUMP_COMMAND]: "science_interfaces/srv/EffortCommand",
  [RosService.DIAPHRAGM_PUMP_COMMAND]: "science_interfaces/srv/EffortCommand",
  [RosService.SCIMBAL_COMMAND]: 'science_interfaces/srv/MoveScimbalCam',
  [RosService.TAKE_NIR_PROBE_READING]: "science_interfaces/srv/TakeNIRProbeReading",
  [RosService.THETA_360_CAM_CAPTURE]: "std_srvs/srv/Trigger",
  [RosService.CALL_RAMAN_SPEC]: "science_interfaces/srv/RamanSpec",
  [RosService.CALL_RAMAN_MECH]: "science_interfaces/srv/RamanMech",
  [RosService.LEDS]: "science_interfaces/srv/SetNamedBool",
  [RosService.CACHE_LEFT_POSITION]: "science_interfaces/srv/SetPosition",
  [RosService.CACHE_LEFT_TWITCH]: "science_interfaces/srv/SetPosition",
  [RosService.CACHE_RIGHT_POSITION]: "science_interfaces/srv/SetPosition",
  [RosService.CACHE_RIGHT_TWITCH]: "science_interfaces/srv/SetPosition",
  [RosService.HEATER]: "science_interfaces/srv/ThermalCommand",
  [RosService.REQUEST_HYDRAPROBE_READING]: "std_srvs/srv/Trigger",
  [RosService.CAROUSEL_INNER_SET_POSITION]: "science_interfaces/srv/SetPosition",
  [RosService.CAROUSEL_OUTER_SET_POSITION]: "science_interfaces/srv/SetPosition",
  [RosService.CAROUSEL_INNER_TRIGGER_ZERO]: "std_srvs/srv/Trigger",
  [RosService.CAROUSEL_OUTER_TRIGGER_ZERO]: "std_srvs/srv/Trigger",
  [RosService.CAROUSEL_INNER_INCREMENT_ZERO]: "science_interfaces/srv/IncrementZero",
  [RosService.CAROUSEL_OUTER_INCREMENT_ZERO]: "science_interfaces/srv/IncrementZero",
  [RosService.LITMUS_DIPPER_DIP]: "science_interfaces/srv/RunPump",
  [RosService.LITMUS_DIPPER_STOP]: "std_srvs/srv/Trigger",
  [RosService.LITMUS_DIPPER_TWITCH]: "science_interfaces/srv/SetPosition",
  [RosService.ZERO_ANALYSIS_ARM]: "std_srvs/srv/Trigger",
  [RosService.SET_AA_POSITION]: "science_interfaces/srv/SetPosition",
  [RosService.STOP_AA_MOVEMENT]:  "std_srvs/srv/Trigger",
  [RosService.RESET_TOF]: "std_srvs/srv/Trigger",
  [RosService.TOOL_ROTATOR_PRESETS]: "science_interfaces/srv/SetNamedPositions",
  [RosService.TOOL_ROTATOR_POSITION]:"science_interfaces/srv/SetPosition",
  [RosService.POWER_CYCLE_SCIENCE]: "science_interfaces/srv/PowerCycleScience",
  [RosService.TOOL_ROTATOR_TWITCH]: "science_interfaces/srv/SetPosition",
  [RosService.PUMPS_RUN]: "science_interfaces/srv/RunPump",
  [RosService.PUMPS_STOP]: "std_srvs/srv/Trigger",
  [RosService.UV_VIS_SPEC_STOP]: "std_srvs/srv/Trigger",
  [RosService.UV_VIS_SPEC_START]: "std_srvs/srv/Trigger",
  [RosService.BME_TRIGGER]: "std_srvs/srv/Trigger",
  [RosService.POTENTIOSTAT_TRIGGER]: "science_interfaces/srv/TriggerOption",

  // General
  [RosService.RGBInput]: "nova_interfaces/srv/RGBInput",

  // Autonomous Related
  [RosService.CARTOGRAPHER_COMMAND]: "nova_interfaces/srv/CartographerCommand",
};
