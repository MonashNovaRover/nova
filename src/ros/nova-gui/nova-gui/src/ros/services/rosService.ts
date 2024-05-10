/**
 * A Single Enum which Represents all ROS Services that Bifrost can call
 *
 * To Call to New Services from Bifrost, Add thhe Service as an enum below
 *
 */

export enum RosService {
  NULL_SERVICE = "",
  GET_IP_LIST = "/camera_streamer/get_host_ip",

  // Arm Related
  READ_RFID = "/electronics/rfid/read",

  // Cameras Related
  START_CAMS = "/camera_streamer/stream/start",
  PAUSE_CAMS = "/camera_streamer/stream/pause",
  BLCMD_RESET = "/control/blcmd_reset",
  
  // Science Related
  KILN_COMMAND = "/science/kiln_command",
  SET_NIR_PROBE_LED = "/science/set_nir_probe_led",
  MOVE_MICROSCOPE_SERVO = "/science/microscope_servo_service",
  CALL_RAMAN_SPEC = "/science/raman_spec_srv",
  CALL_RAMAN_MECH = "/science/raman_mech_srv",
}
