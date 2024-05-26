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
  IRosNovaInterfacesRamanState,
  IRosNovaInterfacesRamanMechResponse,
  IRosNovaInterfacesUvVisSpecData,
  IRosSensorMsgsCompressedImage,
  IRosNovaInterfacesHydraprobeData,
} from "../ros/rosTypes";

import { BifrostStatus } from "./models/bifrost/BifrostTypes";
import { CameraStreamerState } from "./models/CameraStreamState";

import { UIState } from "./models/UIState";

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

  // Camera Stores
  camerasStore: IRosCameraMsgsCameras;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Error Related Stores
  blcmdStatusStore: IRosBlcmdInterfacesBlcmdStatusArray;
  
  // Science Stores
  tofStore: IRosSensorMsgsRange;
  nirStore: IRosNovaInterfacesNirProbeData;
  kilnData: IRosNovaInterfacesKilnData;
  kilnCommand: IRosNovaInterfacesKilnCommandResponse;
  uvVisSpecStore: IRosNovaInterfacesUvVisSpecData;
  microscopeServoStore: IRosNovaInterfacesMicroscopeServoInfo;
  microscopeServiceStore: IRosNovaInterfacesMoveMicroscopeServoResponse;
  ramanSpecServiceStore: IRosNovaInterfacesRamanSpecResponse;
  ramanSpecMessageStore: IRosNovaInterfacesRamanSpectrum;
  ramanMechMessageStore: IRosNovaInterfacesRamanState;
  ramanMechServiceStore: IRosNovaInterfacesRamanMechResponse;
  hydraprobeData: IRosNovaInterfacesHydraprobeData;
  theta360CamStore: IRosSensorMsgsCompressedImage;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;

}
