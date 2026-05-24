import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosBlcmdInterfacesBlcmdStatusArray,
  IRosDriveInterfacesDriveInfo,
  IRosScienceInterfacesEffortStatus,
  IRosScienceInterfacesMicroscopeServoInfo,
  IRosScienceInterfacesNirProbeData,
  IRosScienceInterfacesThermalCommandResponse,
  IRosScienceInterfacesThermalData,
  IRosScienceInterfacesRamanSpecResponse,
  IRosScienceInterfacesRamanSpectrum,
  IRosBlcmdInterfacesTelemetry,
  IRosStdMsgsString,
  IRosStdMsgsBool,
  IRosStdMsgsFloat64,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosScienceInterfacesRamanState,
  IRosScienceInterfacesRamanMechResponse,
  IRosScienceInterfacesUvVisSpecData,
  IRosSensorMsgsCompressedImage,
  IRosScienceInterfacesHydraprobeData,
  IRosScienceInterfacesBmeSensor,
  IRosSensorMsgsBatteryState,
  IRosNovaInterfacesActiveNodeStatus,
  IRosArmInterfacesStringTriggerResponse,
  IRosArmInterfacesKeyboardPoints,
  IRosNovaInterfacesCartographerCommandResponse,
  IRosNovaInterfacesStatus,
  IRosArmInterfacesSequencerFeedback,
  IRosNovaInterfacesRadioStatus,
  IRosSensorMsgsJointState,
  IRosRclInterfacesSetParametersResponse,
  IRosNovaInterfacesLockedStatus,
  IRosScienceInterfacesPumpStatus,
  IRosNovaInterfacesGpsData,
  IRosScienceInterfacesCarouselFeedback,
  IRosScienceInterfacesPotentiostatData,
  IRosScienceInterfacesNamedBools,
} from "../ros/rosTypes";

import { BifrostStatus } from "./models/bifrost/BifrostTypes";
import { CameraStreamerState } from "./models/CameraStreamState";
import { CartographerState } from "./models/CartographerState";

import { UIState } from "./models/UIState";
import { LocalStorageState } from "./models/LocalStorageState.ts";
import { GenericStoreState } from "./models/genericStores/GenericStoreState.ts";
import {Site} from "./models/genericStores/CurrentSiteStore.ts";
import {SiteDataState} from "./models/genericStores/SiteDataState.ts";
import {NIRProbeCalibrationData} from "./models/genericStores/NIRProbeCalibrationData.ts";
import {PresetPositions} from "../components/science/ToolRotatorWidget/ToolRotatorWidget.tsx";

export interface RootState {
  // Bifrost Stores
  bifrostStatus: BifrostStatus;
  poseStore: IRosGeometryMsgsPose;

  // Radio Stores
  radioStore: IRosNovaInterfacesRadioStatus;

  // Drive Stores
  driveStore: IRosDriveInterfacesDriveInfo;
  driveTelemetryStore: IRosBlcmdInterfacesTelemetry;
  driveJointStateStore: IRosSensorMsgsJointState;
  driveTeleopSetParamResponseStore: IRosRclInterfacesSetParametersResponse,

  // Arm Stores
  armTelemetryStore: IRosCmdInterfacesCmDsFeedback;
  rfidDataStore: IRosStdMsgsString;
  keyboardTFTrigger: IRosArmInterfacesStringTriggerResponse;
  keyboardDataStore: IRosArmInterfacesKeyboardPoints;
  sequencerDataStore: IRosArmInterfacesSequencerFeedback;

  // Camera Stores
  camerasStore: IRosCameraMsgsCameras;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Error Related Stores
  blcmdStatusStore: IRosBlcmdInterfacesBlcmdStatusArray;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
  localStorageState: LocalStorageState;

  // Science Stores
  tofStore: IRosSensorMsgsRange;
  aaPosStore: IRosSensorMsgsRange;
  nirStore: IRosScienceInterfacesNirProbeData;
  waterPumpStatus: IRosScienceInterfacesEffortStatus;
  diaphragmPumpStatus: IRosScienceInterfacesEffortStatus;
  thermalData: IRosScienceInterfacesThermalData;
  thermalCommand: IRosScienceInterfacesThermalCommandResponse;
  uvVisSpecStore: IRosScienceInterfacesUvVisSpecData;
  microscopeServoStore: IRosScienceInterfacesMicroscopeServoInfo;
  ramanSpecServiceStore: IRosScienceInterfacesRamanSpecResponse;
  ramanSpecMessageStore: IRosScienceInterfacesRamanSpectrum;
  ramanMechMessageStore: IRosScienceInterfacesRamanState;
  ramanMechServiceStore: IRosScienceInterfacesRamanMechResponse;
  hydraprobeData: IRosScienceInterfacesHydraprobeData;
  theta360CamStore: IRosSensorMsgsCompressedImage;
  bmeSensorStore: IRosScienceInterfacesBmeSensor;
  augerLeftDepthStore: IRosStdMsgsBool;
  augerRightDepthStore: IRosStdMsgsBool;
  toolRotatorAngleStore: IRosStdMsgsFloat64;
  pumpsStatusStore: IRosScienceInterfacesPumpStatus;
  carouselInnerFeedback: IRosScienceInterfacesCarouselFeedback;
  carouselOuterFeedback: IRosScienceInterfacesCarouselFeedback;
  litmusDipperStatusStore: IRosScienceInterfacesPumpStatus;
  potentiostatStore: IRosScienceInterfacesPotentiostatData;
  ledStatusStore: IRosScienceInterfacesNamedBools;

  // Maps Related Stores
  roverLocationStore: IRosNovaInterfacesGpsData;
  baseLocationStore: IRosNovaInterfacesGpsData;
  droneLocationStore: IRosNovaInterfacesGpsData;
  cartographerState: CartographerState;
  cartographerCommand: IRosNovaInterfacesCartographerCommandResponse;
  autoStatus: IRosNovaInterfacesStatus;

  // Generic Stores
  currentSite: GenericStoreState<Site>;
  siteData: GenericStoreState<SiteDataState>;
  nirProbeCalibrationData: GenericStoreState<NIRProbeCalibrationData>
  counter: GenericStoreState<number>;
  rgbLedStore: GenericStoreState<{ r: string; g: string; b: string }>;
  scimbalStepSize : GenericStoreState<string>;
  targetTemp : GenericStoreState<number>;
  waterPumpEffort: GenericStoreState<number>,
  diaphragmPumpEffort: GenericStoreState<number>,
  theta360CompassHeading : GenericStoreState<number>;
  uvVisBlankStore : GenericStoreState<number[]>;
  clickAndHold : GenericStoreState<boolean>;
  windowWideWASD : GenericStoreState<boolean>;
  yoloActiveModel: GenericStoreState<string>;
  yoloUseWebGPU: GenericStoreState<boolean>;
  yoloTimingLogs: GenericStoreState<boolean>;
  toolRotatorPresets: GenericStoreState<PresetPositions>
  toolRotatorTwitchStep: GenericStoreState<number>;
  pumpDefaultDurations: GenericStoreState<Record<string, number>>;
  litmusDipperDefaultDuration: GenericStoreState<number>;
  litmusDipperTwitchStep: GenericStoreState<number>;

  batteryStore: IRosSensorMsgsBatteryState;

  activeStatusStore: IRosNovaInterfacesActiveNodeStatus;
  lockedStatusStore: IRosNovaInterfacesLockedStatus;
}
