import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreBlcmdStatusArray,
  IRosCoreDriveInfo,
  IRosCoreMicroscopeServoInfo,
  IRosCoreMoveMicroscopeServoResponse,
  IRosCoreNirProbeData,
  IRosCoreKilnCommandResponse,
  IRosCoreKilnData,
  IRosCoreRamanSpecResponse,
  IRosCoreRamanSpectrum,
  IRosCoreTelemetry,
  IRosStdMsgsString,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCoreCmDsFeedback 
} from "../ros/rosTypes";

import { BifrostStatus } from "./models/bifrost/BifrostTypes";
import { CameraStreamerState } from "./models/CameraStreamState";

import { UIState } from "./models/UIState";

export interface RootState {
  // Bifrost Stores
  bifrostStatus: BifrostStatus;
  poseStore: IRosGeometryMsgsPose;

  // Drive Stores
  driveStore: IRosCoreDriveInfo;
  driveTelemetryStore: IRosCoreTelemetry;

  // Arm Stores
  armTelemetryStore: IRosCoreCmDsFeedback;
  rfidDataStore: IRosStdMsgsString;

  // Camera Stores
  camerasStore: IRosCameraMsgsCameras;
  ipList: IRosCameraMsgsGetIpListResponse;
  ramanSpecServiceStore: IRosCoreRamanSpecResponse;
  ramanSpecMessageStore: IRosCoreRamanSpectrum;

  // Error Related Stores
  blcmdStatusStore: IRosCoreBlcmdStatusArray;
  
  // Science Stores
  tofStore: IRosSensorMsgsRange;
  nirStore: IRosCoreNirProbeData;
  kilnData: IRosCoreKilnData;
  kilnCommand: IRosCoreKilnCommandResponse;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;

  // Science Stores
  microscopeServoStore: IRosCoreMicroscopeServoInfo;
  microscopeServiceStore: IRosCoreMoveMicroscopeServoResponse;
}
