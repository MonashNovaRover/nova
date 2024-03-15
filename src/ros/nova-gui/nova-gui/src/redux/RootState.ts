import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreBlcmdStatusArray,
  IRosCoreDriveInfo,
  IRosCoreNirProbeData,
  IRosCoreKilnCommandResponse,
  IRosCoreKilnData,
  IRosCoreTelemetry,
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
  armTelemetryStore: IRosCoreCmDsFeedback ;

  // Camera Stores
  camerasStore: IRosCameraMsgsCameras;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Error Related Stores
  blcmdStatusStore: IRosCoreBlcmdStatusArray;
  
  // Science Bifrost states
  nirStore: IRosCoreNirProbeData;
  kilnData: IRosCoreKilnData;
  kilnCommand: IRosCoreKilnCommandResponse;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
}
