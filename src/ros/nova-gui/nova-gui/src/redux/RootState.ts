import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreDriveInfo,
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
  kilnData: IRosCoreKilnData;
  kilnCommand: IRosCoreKilnCommandResponse;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
}
