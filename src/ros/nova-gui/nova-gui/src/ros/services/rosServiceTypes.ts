import {
  IRosCameraMsgsGetIpListResponse,
  IRosCoreSetNirProbeLedRequest,
  IRosCoreSetNirProbeLedResponse
} from "../rosTypes";
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
  [RosService.SET_NIR_PROBE_LED]: RosServiceMessage<
    IRosCoreSetNirProbeLedRequest,
    IRosCoreSetNirProbeLedResponse
  >;
}
