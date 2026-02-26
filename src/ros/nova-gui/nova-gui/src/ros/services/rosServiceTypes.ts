import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosBlcmdInterfacesBlcmdResetRequest,
  IRosBlcmdInterfacesBlcmdResetResponse,
  IRosScienceInterfacesMoveMicroscopeServoRequest,
  IRosScienceInterfacesMoveMicroscopeServoResponse,
  IRosArmInterfacesTypeSequenceRequest,
  IRosArmInterfacesTypeSequenceResponse,
  IRosStdSrvsTriggerResponse,
  IRosScienceInterfacesKilnCommandRequest,
  IRosScienceInterfacesKilnCommandResponse,
  IRosScienceInterfacesEffortCommandRequest,
  IRosScienceInterfacesEffortCommandResponse,
  IRosScienceInterfacesRamanSpecRequest,
  IRosScienceInterfacesRamanSpecResponse,
  IRosScienceInterfacesRamanMechRequest,
  IRosScienceInterfacesRamanMechResponse,
  IRosCameraMsgsGetIpListResponse,
  IRosStdSrvsSetBoolResponse,
  IRosStdSrvsSetBoolRequest,
  IRosScienceInterfacesMoveScimbalCamRequest,
  IRosScienceInterfacesMoveScimbalCamResponse,
  IRosScienceInterfacesTakeNirProbeReadingRequest,
  IRosScienceInterfacesTakeNirProbeReadingResponse,
  IRosNovaInterfacesRgbInputRequest,
  IRosNovaInterfacesRgbInputResponse,
  IRosScienceInterfacesCacheCommandRequest,
  IRosScienceInterfacesCacheCommandResponse,
  IRosNovaInterfacesCartographerCommandRequest,
  IRosNovaInterfacesCartographerCommandResponse,
  IRosScienceInterfacesMoveHydraprobeRequest,
  IRosScienceInterfacesMoveHydraprobeResponse, IRosScienceInterfacesSetPositionRequest,
  IRosScienceInterfacesSetPositionResponse,
} from "../rosTypes";
import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

interface EmptyMessage {}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<EmptyMessage, EmptyMessage>;

  // Arm related
  [RosService.READ_RFID]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.START_AUTO_TYPING]: RosServiceMessage<
    IRosArmInterfacesTypeSequenceRequest,
    IRosArmInterfacesTypeSequenceResponse
  >;
  [RosService.STOP_AUTO_TYPING]: RosServiceMessage<
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
    IRosScienceInterfacesKilnCommandRequest,
    IRosScienceInterfacesKilnCommandResponse
  >;
  [RosService.WATER_PUMP_COMMAND]: RosServiceMessage<
    IRosScienceInterfacesEffortCommandRequest,
    IRosScienceInterfacesEffortCommandResponse
  >;
  [RosService.DIAPHRAGM_PUMP_COMMAND]: RosServiceMessage<
    IRosScienceInterfacesEffortCommandRequest,
    IRosScienceInterfacesEffortCommandResponse
  >;
  [RosService.TAKE_NIR_PROBE_READING]: RosServiceMessage<
    IRosScienceInterfacesTakeNirProbeReadingRequest,
    IRosScienceInterfacesTakeNirProbeReadingResponse
  >;
  [RosService.SCIMBAL_COMMAND]: RosServiceMessage<
    IRosScienceInterfacesMoveScimbalCamRequest,
    IRosScienceInterfacesMoveScimbalCamResponse
  >;
  [RosService.HYDRAPROBE_COMMAND]: RosServiceMessage<
    IRosScienceInterfacesMoveHydraprobeRequest,
    IRosScienceInterfacesMoveHydraprobeResponse
  >;
  [RosService.MOVE_MICROSCOPE_SERVO]: RosServiceMessage<
    IRosScienceInterfacesMoveMicroscopeServoRequest,
    IRosScienceInterfacesMoveMicroscopeServoResponse
  >;
  [RosService.THETA_360_CAM_CAPTURE]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CALL_RAMAN_SPEC]: RosServiceMessage<
    IRosScienceInterfacesRamanSpecRequest,
    IRosScienceInterfacesRamanSpecResponse
  >;
  [RosService.CALL_RAMAN_MECH]: RosServiceMessage<
    IRosScienceInterfacesRamanMechRequest,
    IRosScienceInterfacesRamanMechResponse
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
    IRosScienceInterfacesCacheCommandRequest,
    IRosScienceInterfacesCacheCommandResponse
  >;
  [RosService.CACHE_2]: RosServiceMessage<
    IRosScienceInterfacesCacheCommandRequest,
    IRosScienceInterfacesCacheCommandResponse
  >;
  [RosService.HEATER]: RosServiceMessage<
    IRosScienceInterfacesKilnCommandRequest,
    IRosScienceInterfacesKilnCommandResponse
  >;
  [RosService.REQUEST_HYDRAPROBE_READING]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CAROUSEL]: RosServiceMessage<
    IRosScienceInterfacesKilnCommandRequest,
    IRosScienceInterfacesKilnCommandResponse
  >;
  [RosService.ZERO_ANALYSIS_ARM]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.SET_AA_POSITION]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.RESET_TOF]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;

  // General
  [RosService.RGBInput]: RosServiceMessage<
    IRosNovaInterfacesRgbInputRequest,
    IRosNovaInterfacesRgbInputResponse
  >;

  // Autonomous Related
  [RosService.CARTOGRAPHER_COMMAND]: RosServiceMessage<
    IRosNovaInterfacesCartographerCommandRequest,
    IRosNovaInterfacesCartographerCommandResponse
  >;
}
