/**
 * A Single Enum which Represents all ROS Services that Bifrost can call
 *
 * To Call to New Services from Bifrost, Add thhe Service as an enum below
 *
 */

export enum RosService {
  NULL_SERVICE = "",
  GET_IP_LIST = "/camera_streamer/get_host_ip",
  SET_NIR_PROBE_LED = "/science/set_nir_probe_led",
  START_CAMS = "/camera_streamer/stream/start",
  PAUSE_CAMS = "/camera_streamer/stream/pause",
}
