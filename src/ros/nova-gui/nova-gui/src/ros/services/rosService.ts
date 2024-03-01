/**
 * A Single Enum which Represents all ROS Services that Bifrost can call
 *
 * To Call to New Services from Bifrost, Add thhe Service as an enum below
 *
 */

export enum RosService {
  NULL_SERVICE = "",
  GET_IP_LIST = "/camera_streamer/get_host_ip",
  READ_RFID = "/electronics/rfid/read",
}
