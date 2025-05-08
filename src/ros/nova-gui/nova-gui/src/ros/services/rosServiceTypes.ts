import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosBlcmdInterfacesBlcmdResetRequest,
  IRosBlcmdInterfacesBlcmdResetResponse,
  IRosNovaInterfacesMoveMicroscopeServoRequest,
  IRosNovaInterfacesMoveMicroscopeServoResponse,
  IRosStdSrvsTriggerResponse,
  IRosNovaInterfacesKilnCommandRequest,
  IRosNovaInterfacesKilnCommandResponse,
  IRosNovaInterfacesRamanSpecRequest,
  IRosNovaInterfacesRamanSpecResponse,
  IRosNovaInterfacesRamanMechRequest,
  IRosNovaInterfacesRamanMechResponse,
  IRosCameraMsgsGetIpListResponse,
  IRosStdSrvsSetBoolResponse,
  IRosStdSrvsSetBoolRequest,
  IRosNovaInterfacesMoveScimbalCamRequest,
  IRosNovaInterfacesMoveScimbalCamResponse,
  IRosNovaInterfacesTakeNirProbeReadingRequest,
  IRosNovaInterfacesTakeNirProbeReadingResponse,
  IRosNovaInterfacesCacheCommandRequest,
  IRosNovaInterfacesCacheCommandResponse,

} from "../rosTypes";
import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

interface EmptyMessage {}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<EmptyMessage, EmptyMessage>;
  
  
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
  [RosService.GET_IP_LIST]: RosServiceMessage<
    EmptyMessage,
    IRosCameraMsgsGetIpListResponse
  >;

  // Error Related
  [RosService.BLCMD_RESET]: RosServiceMessage<
    IRosBlcmdInterfacesBlcmdResetRequest,
    IRosBlcmdInterfacesBlcmdResetResponse
  >;

  // Science Related
  [RosService.MIXERS]: RosServiceMessage<
    IRosStdSrvsSetBoolRequest,
    IRosStdSrvsSetBoolResponse
  >;
  [RosService.KILN_COMMAND]: RosServiceMessage<
    IRosNovaInterfacesKilnCommandRequest,
    IRosNovaInterfacesKilnCommandResponse
  >;
  [RosService.TAKE_NIR_PROBE_READING]: RosServiceMessage<
    IRosNovaInterfacesTakeNirProbeReadingRequest,
    IRosNovaInterfacesTakeNirProbeReadingResponse
  >;
  [RosService.SCIMBAL_COMMAND]: RosServiceMessage<
    IRosNovaInterfacesMoveScimbalCamRequest,
    IRosNovaInterfacesMoveScimbalCamResponse
  >;
  [RosService.MOVE_MICROSCOPE_SERVO]: RosServiceMessage<
    IRosNovaInterfacesMoveMicroscopeServoRequest,
    IRosNovaInterfacesMoveMicroscopeServoResponse
  >;
  [RosService.THETA_360_CAM_CAPTURE]: RosServiceMessage<
    EmptyMessage, 
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CALL_RAMAN_SPEC]: RosServiceMessage<
    IRosNovaInterfacesRamanSpecRequest,
    IRosNovaInterfacesRamanSpecResponse
  >;
  [RosService.CALL_RAMAN_MECH]: RosServiceMessage<
    IRosNovaInterfacesRamanMechRequest,
    IRosNovaInterfacesRamanMechResponse
  >;
  [RosService.UV_VIS_LED_1]: RosServiceMessage<
    IRosStdSrvsSetBoolRequest,
    IRosStdSrvsSetBoolResponse
  >;
  [RosService.UV_VIS_LED_2]: RosServiceMessage<
    IRosStdSrvsSetBoolRequest,
    IRosStdSrvsSetBoolResponse
  >;
  [RosService.CACHE_1]: RosServiceMessage<
    IRosNovaInterfacesCacheCommandRequest,
    IRosNovaInterfacesCacheCommandResponse
  >;
  [RosService.CACHE_2]: RosServiceMessage<
    IRosNovaInterfacesCacheCommandRequest,
    IRosNovaInterfacesCacheCommandResponse
  >;
  [RosService.HEATER]: RosServiceMessage<
      IRosStdSrvsSetBoolRequest,
      IRosStdSrvsSetBoolResponse
  >;
}
