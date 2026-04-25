import {
  IRosCameraMsgsCameraOperationRequest,
  IRosCameraMsgsCameraOperationResponse,
  IRosBlcmdInterfacesBlcmdResetRequest,
  IRosBlcmdInterfacesBlcmdResetResponse,
  IRosArmInterfacesTypeSequenceRequest,
  IRosArmInterfacesTypeSequenceResponse,
  IRosStdSrvsTriggerResponse,
  IRosScienceInterfacesThermalCommandRequest,
  IRosScienceInterfacesThermalCommandResponse,
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
  IRosScienceInterfacesSetPositionRequest,
  IRosScienceInterfacesSetPositionResponse,
  IRosScienceInterfacesPowerCycleRequest,
  IRosScienceInterfacesPowerCycleResponse,
  IRosRclInterfacesSetParametersRequest,
  IRosRclInterfacesSetParametersResponse,
  IRosScienceInterfacesRunPumpRequest,
  IRosScienceInterfacesRunPumpResponse,
  IRosCameraMsgsCameraProfileSelectionRequest,
  IRosCameraMsgsCameraProfileSelectionResponse,
  IRosScienceInterfacesSetNamedPositionsRequest,
  IRosScienceInterfacesSetNamedPositionsResponse,
  IRosScienceInterfacesIncrementZeroRequest,
  IRosScienceInterfacesIncrementZeroResponse,
  IRosScienceInterfacesSetNamedBoolRequest,
  IRosScienceInterfacesSetNamedBoolResponse,
} from "../rosTypes";
import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

interface EmptyMessage {}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<EmptyMessage, EmptyMessage>;

  // Drive Related
  [RosService.TELEOP_DRIVE_SET_PARAMS]: RosServiceMessage<
    IRosRclInterfacesSetParametersRequest,
    IRosRclInterfacesSetParametersResponse
  >;

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
  [RosService.STOP_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraOperationRequest,
    IRosCameraMsgsCameraOperationResponse
  >;
  [RosService.PRESET_CAMS]: RosServiceMessage<
    IRosCameraMsgsCameraProfileSelectionRequest,
    IRosCameraMsgsCameraProfileSelectionResponse
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
  [RosService.THERMAL_COMMAND]: RosServiceMessage<
    IRosScienceInterfacesThermalCommandRequest,
    IRosScienceInterfacesThermalCommandResponse
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
  [RosService.LEDS]: RosServiceMessage<
    IRosScienceInterfacesSetNamedBoolRequest,
    IRosScienceInterfacesSetNamedBoolResponse
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
    IRosScienceInterfacesThermalCommandRequest,
    IRosScienceInterfacesThermalCommandResponse
  >;
  [RosService.REQUEST_HYDRAPROBE_READING]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CAROUSEL_INNER_SET_POSITION]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.CAROUSEL_OUTER_SET_POSITION]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.CAROUSEL_INNER_TRIGGER_ZERO]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CAROUSEL_OUTER_TRIGGER_ZERO]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.CAROUSEL_INNER_INCREMENT_ZERO]: RosServiceMessage<
    IRosScienceInterfacesIncrementZeroRequest,
    IRosScienceInterfacesIncrementZeroResponse
  >;
  [RosService.CAROUSEL_OUTER_INCREMENT_ZERO]: RosServiceMessage<
    IRosScienceInterfacesIncrementZeroRequest,
    IRosScienceInterfacesIncrementZeroResponse
  >;
  [RosService.LITMUS_DIPPER_DIP]: RosServiceMessage<
    IRosScienceInterfacesRunPumpRequest,
    IRosScienceInterfacesRunPumpResponse
  >;
  [RosService.LITMUS_DIPPER_STOP]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.LITMUS_DIPPER_TWITCH]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.ZERO_ANALYSIS_ARM]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.SET_AA_POSITION]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.STOP_AA_MOVEMENT]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.RESET_TOF]: RosServiceMessage<
    EmptyMessage,
    IRosStdSrvsTriggerResponse
  >;
  [RosService.TOOL_ROTATOR_PRESETS]: RosServiceMessage<
    IRosScienceInterfacesSetNamedPositionsRequest,
    IRosScienceInterfacesSetNamedPositionsResponse>;
  [RosService.TOOL_ROTATOR_POSITION]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.TOOL_ROTATOR_TWITCH]: RosServiceMessage<
    IRosScienceInterfacesSetPositionRequest,
    IRosScienceInterfacesSetPositionResponse
  >;
  [RosService.POWER_CYCLE_SCIENCE]:  RosServiceMessage<
    IRosScienceInterfacesPowerCycleRequest,
    IRosScienceInterfacesPowerCycleResponse
  >;
  [RosService.PUMPS_RUN]: RosServiceMessage<
    IRosScienceInterfacesRunPumpRequest,
    IRosScienceInterfacesRunPumpResponse
  >;
  [RosService.PUMPS_STOP]: RosServiceMessage<
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
