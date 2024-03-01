import { IRosCameraMsgsGetIpListResponse } from "../rosTypes";
import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

interface EmptyMessage {}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<EmptyMessage, EmptyMessage>;
  [RosService.GET_IP_LIST]: RosServiceMessage<
    EmptyMessage,
    IRosCameraMsgsGetIpListResponse
  >;
  [RosService.READ_RFID]: RosServiceMessage<EmptyMessage, EmptyMessage>
}
