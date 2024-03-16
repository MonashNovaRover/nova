import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreDriveInfo,
  IRosCoreMicroscopeServoInfo,
  IRosCoreMoveMicroscopeServoResponse,
  IRosCoreNirProbeData,
  IRosCoreKilnCommandResponse,
  IRosCoreKilnData,
  IRosCoreTelemetry,
  IRosStdMsgsString,
  IRosGeometryMsgsPose,
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

  // Science Bifrost states
  nirStore: IRosCoreNirProbeData;
  kilnData: IRosCoreKilnData;
  kilnCommand: IRosCoreKilnCommandResponse;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;

  // Science Stores
  microscopeServoState: IRosCoreMicroscopeServoInfo;
  microscopeServoService: IRosCoreMoveMicroscopeServoResponse;
}
