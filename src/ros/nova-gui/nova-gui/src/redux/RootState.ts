import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosBlcmdInterfacesBlcmdStatusArray,
  IRosDriveInterfacesDriveInfo,
  IRosNovaInterfacesMicroscopeServoInfo,
  IRosNovaInterfacesMoveMicroscopeServoResponse,
  IRosNovaInterfacesNirProbeData,
  IRosNovaInterfacesKilnCommandResponse,
  IRosNovaInterfacesKilnData,
  IRosNovaInterfacesRamanSpecResponse,
  IRosNovaInterfacesRamanSpectrum,
  IRosBlcmdInterfacesTelemetry,
  IRosStdMsgsString,
  IRosStdMsgsBool,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosNovaInterfacesRamanState,
  IRosNovaInterfacesRamanMechResponse,
  IRosNovaInterfacesUvVisSpecData,
  IRosSensorMsgsCompressedImage,
  IRosNovaInterfacesHydraprobeData,
  IRosStdSrvsSetBoolResponse,
  IRosNovaInterfacesBmeSensor,
  IRosSensorMsgsBatteryState,
  IRosNovaInterfacesActiveNodeStatus,
  IRosArmInterfacesStringTriggerResponse,
  IRosArmInterfacesKeyboardPoints,
  IRosNovaInterfacesCartographerCommandResponse,
  IRosNovaInterfacesStatus,
  IRosSensorMsgsNavSatFix,
  IRosArmInterfacesSequencerFeedback,

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

  // Drive Stores
  driveStore: IRosDriveInterfacesDriveInfo;
  driveTelemetryStore: IRosBlcmdInterfacesTelemetry;

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
  nirStore: IRosNovaInterfacesNirProbeData;
  kilnData: IRosNovaInterfacesKilnData;
  kilnCommand: IRosNovaInterfacesKilnCommandResponse;
  uvVisSpecStore: IRosNovaInterfacesUvVisSpecData;
  uvVisLED1Store: IRosStdSrvsSetBoolResponse;
  uvVisLED2Store: IRosStdSrvsSetBoolResponse;
  microscopeServoStore: IRosNovaInterfacesMicroscopeServoInfo;
  microscopeServiceStore: IRosNovaInterfacesMoveMicroscopeServoResponse;
  ramanSpecServiceStore: IRosNovaInterfacesRamanSpecResponse;
  ramanSpecMessageStore: IRosNovaInterfacesRamanSpectrum;
  ramanMechMessageStore: IRosNovaInterfacesRamanState;
  ramanMechServiceStore: IRosNovaInterfacesRamanMechResponse;
  hydraprobeData: IRosNovaInterfacesHydraprobeData;
  theta360CamStore: IRosSensorMsgsCompressedImage;
  bmeSensorStore: IRosNovaInterfacesBmeSensor;
  auger1DepthSensorStore: IRosStdMsgsBool;
  auger2DepthSensorStore: IRosStdMsgsBool;

  // maps Related Stores
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
  theta360CompassHeading : GenericStoreState<number>;
  uvVisBlankStore : GenericStoreState<number[]>;

  batteryStore: IRosSensorMsgsBatteryState;

  activeStatusStore: IRosNovaInterfacesActiveNodeStatus;
}
