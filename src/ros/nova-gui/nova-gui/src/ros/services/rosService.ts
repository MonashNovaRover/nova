/**
 * A Single Enum which Represents all ROS Services that Bifrost can call
 *
 * To Call to New Services from Bifrost, Add thhe Service as an enum below
 *
 */

export enum RosService {
  NULL_SERVICE = "",

  // Arm Related
  READ_RFID = "/electronics/rfid/read",
  START_AUTO_TYPING = '/type_sequence/start',
  STOP_AUTO_TYPING = '/type_sequence/stop',

  // Cameras Related
  START_CAMS = "/camera_streamer/stream/start",
  PAUSE_CAMS = "/camera_streamer/stream/pause",
  GET_IP_LIST = "/camera_streamer/get_host_ip",
  BLCMD_RESET = "/blcmds/blcmd_reset",
  
  // Science Related
  MIXERS = "/science/mixers",
  KILN_COMMAND = "/science/kiln_command",
  SCIMBAL_COMMAND = "/science/scimbal_cam_service",
  TAKE_NIR_PROBE_READING = "/science/take_nir_probe_reading",
  MOVE_MICROSCOPE_SERVO = "/science/microscope_servo_service",
  THETA_360_CAM_CAPTURE = "/science/theta360cam/capture",
  CALL_RAMAN_SPEC = "/science/raman_spec_srv",
  CALL_RAMAN_MECH = "/science/raman_mech_srv",
  UV_VIS_LED_1 = "/science/uv_vis_led_1",
  UV_VIS_LED_2 = "/science/uv_vis_led_2",
  CACHE_1 = "/science/cache_command_1",
  CACHE_2 = "/science/cache_command_2",
  HEATER = "/science/heater",
}
