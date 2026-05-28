/**
 * A Single Enum which Represents all ROS Services that Bifrost can call
 *
 * To Call to New Services from Bifrost, Add the Service as an enum below
 *
 */

export enum RosService {
  NULL_SERVICE = "",

  // Drive Related
  TELEOP_DRIVE_SET_PARAMS = "/teleop_drive_joy_node/set_parameters",

  // Arm Related
  READ_RFID = "/electronics/rfid/read",
  START_AUTO_TYPING = '/type_sequence/start',
  STOP_AUTO_TYPING = '/type_sequence/stop',

  // Cameras Related
  START_CAMS = "/camera_streamer/stream/start",
  PAUSE_CAMS = "/camera_streamer/stream/pause",
  STOP_CAMS = "/camera_streamer/stream/stop",
  PRESET_CAMS = "/camera_streamer/stream/profile",
  GET_IP_LIST = "/camera_streamer/get_host_ip",
  BLCMD_RESET = "/blcmds/blcmd_reset",
  
  // Science Related
  THERMAL_COMMAND = "/science/thermal_command",
  WATER_PUMP_COMMAND = "/science/water_pump_command",
  DIAPHRAGM_PUMP_COMMAND = "/science/diaphragm_pump_command",
  SCIMBAL_COMMAND = "/science/scimbal_cam_service",
  TAKE_NIR_PROBE_READING = "/science/take_nir_probe_reading",
  THETA_360_CAM_CAPTURE = "/science/theta360cam/capture",
  CALL_RAMAN_SPEC = "/science/raman_spec_srv",
  CALL_RAMAN_MECH = "/science/raman_mech_srv",
  LEDS = "/science/leds/set",
  CACHE_LEFT_POSITION = "/science/cache_left/set_position",
  CACHE_LEFT_TWITCH = "/science/cache_left/twitch",
  CACHE_RIGHT_POSITION = "/science/cache_right/set_position",
  CACHE_RIGHT_TWITCH = "/science/cache_right/twitch",
  HEATER = "/science/heater",
  CAROUSEL_INNER_SET_POSITION = "/science/carousel_inner/set_position",
  CAROUSEL_OUTER_SET_POSITION = "/science/carousel_outer/set_position",
  CAROUSEL_INNER_TRIGGER_ZERO = "/science/carousel_inner/trigger_zero",
  CAROUSEL_OUTER_TRIGGER_ZERO = "/science/carousel_outer/trigger_zero",
  CAROUSEL_INNER_INCREMENT_ZERO = "/science/carousel_inner/increment_zero",
  CAROUSEL_OUTER_INCREMENT_ZERO = "/science/carousel_outer/increment_zero",
  LITMUS_DIPPER_DIP = "/science/litmus_dipper/dip",
  LITMUS_DIPPER_STOP = "/science/litmus_dipper/stop",
  LITMUS_DIPPER_TWITCH = "/science/litmus_dipper/twitch",
  RGBInput = "/set_RGBInput",
  ZERO_ANALYSIS_ARM = "/science/analysis_arm/zero",
  SET_AA_POSITION = "/science/analysis_arm/set_position",
  STOP_AA_MOVEMENT = "/science/analysis_arm/stop",
  RESET_TOF = "/science/tof/reset",
  TOOL_ROTATOR_PRESETS = "/science/tool_rotator/set_presets",
  TOOL_ROTATOR_POSITION = "/science/tool_rotator/set_position",
  POWER_CYCLE_SCIENCE = "/science/power_cycle",
  TOOL_ROTATOR_TWITCH = "/science/tool_rotator/twitch",
  PUMPS_RUN = "/science/pumps/run",
  PUMPS_STOP = "/science/pumps/stop",
  UV_VIS_SPEC_STOP = "/science/uv_vis_spec/stop",
  UV_VIS_SPEC_START = "/science/uv_vis_spec/start",
  BME_TRIGGER = "/science/bme/trigger",
  POTENTIOSTAT_TRIGGER = "/science/potentiostat/trigger",

  // General
  REQUEST_HYDRAPROBE_READING = "/science/request_hydraprobe_reading",

  // Autononomous Related
  CARTOGRAPHER_COMMAND = "/autonomous/cartographer_command",
  CANCEL_NAVIGATION = "/autonomous/cancel_navigation",
}
