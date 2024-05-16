import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosCameraMsgsGetIpListResponse,
  IRosBlcmdInterfacesBlcmdResetRequest,
  IRosBlcmdInterfacesBlcmdResetResponse,
  IRosNovaInterfacesMoveMicroscopeServoRequest,
  IRosNovaInterfacesMoveMicroscopeServoResponse,
  IRosStdSrvsTriggerResponse,
  IRosNovaInterfacesKilnCommandRequest,
  IRosNovaInterfacesKilnCommandResponse,
  IRosNovaInterfacesSetNirProbeLedRequest,
  IRosNovaInterfacesSetNirProbeLedResponse,
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
  [RosService.READ_RFID]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;

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
    IRosBlcmdInterfacesBlcmdResetRequest,
    IRosBlcmdInterfacesBlcmdResetResponse
  >;

  // Science Related
  [RosService.KILN_COMMAND]: RosServiceMessage<
    IRosNovaInterfacesKilnCommandRequest,
    IRosNovaInterfacesKilnCommandResponse
  >;
  [RosService.SET_NIR_PROBE_LED]: RosServiceMessage<
    IRosNovaInterfacesSetNirProbeLedRequest,
    IRosNovaInterfacesSetNirProbeLedResponse
  >;
  [RosService.MOVE_MICROSCOPE_SERVO]: RosServiceMessage<
    IRosNovaInterfacesMoveMicroscopeServoRequest,
    IRosNovaInterfacesMoveMicroscopeServoResponse
  >;

  [RosService.THETA_360_CAM_CAPTURE]: EmptyMessage;
}
