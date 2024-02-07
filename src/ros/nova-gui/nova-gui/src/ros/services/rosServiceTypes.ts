import { /*IRosCameraMsgsGetIpListResponse,*/ IRosCoreRamanSpecRequest, IRosCoreRamanSpecResponse } from "../rosTypes";
import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

interface EmptyMessage {}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<EmptyMessage, EmptyMessage>;
  /*[RosService.GET_IP_LIST]: RosServiceMessage<
    EmptyMessage,
    IRosCameraMsgsGetIpListResponse
  >;*/
  [RosService.CALL_RAMAN_SPEC]: RosServiceMessage<
    IRosCoreRamanSpecRequest, 
    IRosCoreRamanSpecResponse>;
}
