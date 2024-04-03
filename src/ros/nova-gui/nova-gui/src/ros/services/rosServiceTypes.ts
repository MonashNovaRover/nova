import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreRamanSpecRequest, 
  IRosCoreRamanSpecResponse,
  IRosCoreBlcmdResetRequest,
  IRosCoreBlcmdResetResponse,
  IRosCoreMoveMicroscopeServoRequest,
  IRosCoreMoveMicroscopeServoResponse,
  IRosStdSrvsTriggerResponse,
  IRosCoreKilnCommandRequest,
  IRosCoreKilnCommandResponse,
  IRosCoreSetNirProbeLedRequest,
  IRosCoreSetNirProbeLedResponse,
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
  [RosService.READ_RFID]: RosServiceMessage<EmptyMessage, IRosStdSrvsTriggerResponse>;

  // Camera Related
  [RosService.START_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraOperationRequest,
    IRosCameraMsgsCameraOperationResponse
  >;
  [RosService.PAUSE_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraOperationRequest,
    IRosCameraMsgsCameraOperationResponse
  >;

  // Error Related
  [RosService.BLCMD_RESET]: RosServiceMessage<
    IRosCoreBlcmdResetRequest,
    IRosCoreBlcmdResetResponse
  >;

  // Science Related
  [RosService.KILN_COMMAND]: RosServiceMessage<
    IRosCoreKilnCommandRequest,
    IRosCoreKilnCommandResponse
  >;
  [RosService.SET_NIR_PROBE_LED]: RosServiceMessage<
    IRosCoreSetNirProbeLedRequest,
    IRosCoreSetNirProbeLedResponse
  >;
  [RosService.MOVE_MICROSCOPE_SERVO]: RosServiceMessage<
    IRosCoreMoveMicroscopeServoRequest,
    IRosCoreMoveMicroscopeServoResponse
  >;
  [RosService.CALL_RAMAN_SPEC]: RosServiceMessage<
    IRosCoreRamanSpecRequest, 
    IRosCoreRamanSpecResponse
  >;
}
