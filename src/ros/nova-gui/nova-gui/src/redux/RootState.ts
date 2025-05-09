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
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosSensorMsgsNavSatFix,
  IRosNovaInterfacesRamanState,
  IRosNovaInterfacesRamanMechResponse,
  IRosNovaInterfacesUvVisSpecData,
  IRosSensorMsgsCompressedImage,
  IRosNovaInterfacesHydraprobeData,
  IRosStdSrvsSetBoolResponse,
  IRosNovaInterfacesBmeSensor,
  IRosSensorMsgsBatteryState,
  IRosArmInterfacesStringTriggerResponse,
  IRosNovaInterfacesActiveNodeStatus,
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
  keyboardData: IRosArmInterfacesStringTriggerResponse;

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

  // Maps Related Stores
  roverLocationStore: IRosSensorMsgsNavSatFix;
  baseLocationStore: IRosSensorMsgsNavSatFix;
  cartographerState: CartographerState;

  // Generic Stores
  currentSite: GenericStoreState<Site>;
  siteData: GenericStoreState<SiteDataState>;
  nirProbeCalibrationData: GenericStoreState<NIRProbeCalibrationData>
  counter: GenericStoreState<number>;
  scimbalStepSize : GenericStoreState<string>;
  targetTemp : GenericStoreState<number>;
  theta360CompassHeading : GenericStoreState<number>;

  batteryStore: IRosSensorMsgsBatteryState;
  activeStatusStore: IRosNovaInterfacesActiveNodeStatus;
}
