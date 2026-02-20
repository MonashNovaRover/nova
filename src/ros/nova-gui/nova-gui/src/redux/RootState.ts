import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosBlcmdInterfacesBlcmdStatusArray,
  IRosDriveInterfacesDriveInfo,
  IRosScienceInterfacesEffortStatus,
  IRosScienceInterfacesMicroscopeServoInfo,
  IRosScienceInterfacesMoveMicroscopeServoResponse,
  IRosScienceInterfacesNirProbeData,
  IRosScienceInterfacesKilnCommandResponse,
  IRosScienceInterfacesKilnData,
  IRosScienceInterfacesRamanSpecResponse,
  IRosScienceInterfacesRamanSpectrum,
  IRosBlcmdInterfacesTelemetry,
  IRosStdMsgsString,
  IRosStdMsgsBool,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosScienceInterfacesRamanState,
  IRosScienceInterfacesRamanMechResponse,
  IRosScienceInterfacesUvVisSpecData,
  IRosSensorMsgsCompressedImage,
  IRosScienceInterfacesHydraprobeData,
  IRosStdSrvsSetBoolResponse,
  IRosScienceInterfacesBmeSensor,
  IRosSensorMsgsBatteryState,
  IRosNovaInterfacesActiveNodeStatus,
  IRosArmInterfacesStringTriggerResponse,
  IRosArmInterfacesKeyboardPoints,
  IRosNovaInterfacesCartographerCommandResponse,
  IRosNovaInterfacesStatus,
  IRosSensorMsgsNavSatFix,
  IRosArmInterfacesSequencerFeedback,
  IRosNovaInterfacesRadioStatus, IRosSensorMsgsJointState,

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
  kilnData: IRosScienceInterfacesKilnData;
  kilnCommand: IRosScienceInterfacesKilnCommandResponse;
  uvVisSpecStore: IRosScienceInterfacesUvVisSpecData;
  uvVisLED1Store: IRosStdSrvsSetBoolResponse;
  uvVisLED2Store: IRosStdSrvsSetBoolResponse;
  microscopeServoStore: IRosScienceInterfacesMicroscopeServoInfo;
  microscopeServiceStore: IRosScienceInterfacesMoveMicroscopeServoResponse;
  ramanSpecServiceStore: IRosScienceInterfacesRamanSpecResponse;
  ramanSpecMessageStore: IRosScienceInterfacesRamanSpectrum;
  ramanMechMessageStore: IRosScienceInterfacesRamanState;
  ramanMechServiceStore: IRosScienceInterfacesRamanMechResponse;
  hydraprobeData: IRosScienceInterfacesHydraprobeData;
  theta360CamStore: IRosSensorMsgsCompressedImage;
  bmeSensorStore: IRosScienceInterfacesBmeSensor;
  auger1DepthSensorStore: IRosStdMsgsBool;
  auger2DepthSensorStore: IRosStdMsgsBool;

  // Maps Related Stores
  roverLocationStore: IRosSensorMsgsNavSatFix;
  baseLocationStore: IRosSensorMsgsNavSatFix;
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

  batteryStore: IRosSensorMsgsBatteryState;

  activeStatusStore: IRosNovaInterfacesActiveNodeStatus;
}
