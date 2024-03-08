import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreBlcmdResetRequest,
  IRosCoreBlcmdResetResponse,
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
  [RosService.START_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraOperationRequest,
    IRosCameraMsgsCameraOperationResponse
  >;
  [RosService.PAUSE_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraOperationRequest,
    IRosCameraMsgsCameraOperationResponse
  >;

  [RosService.BLCMD_RESET]: RosServiceMessage<
    IRosCoreBlcmdResetRequest,
    IRosCoreBlcmdResetResponse
  >;
}
