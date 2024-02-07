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
  IRosBlcmdInterfacesTelemetry,
  IRosStdMsgsString,
  IRosCoreRamanSpecResponse,
  IRosCoreRamanSpectrum,
  IRosCoreTelemetry,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosSensorMsgsCompressedImage,
  IRosNovaInterfacesHydraprobeData,
  IRosCmdInterfacesCmDsFeedback
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
  hydraprobeData: IRosNovaInterfacesHydraprobeData;
  microscopeServoStore: IRosNovaInterfacesMicroscopeServoInfo;
  microscopeServiceStore: IRosNovaInterfacesMoveMicroscopeServoResponse;
  theta360CamStore: IRosSensorMsgsCompressedImage;
  ramanSpecServiceStore: IRosCoreRamanSpecResponse;
  ramanSpecMessageStore: IRosCoreRamanSpectrum;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;

}
