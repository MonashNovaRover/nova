import {
  IRosCameraMsgsCameras,
  IRosCameraMsgsGetIpListResponse,
  IRosCoreDriveInfo,
  IRosCoreNirProbeData,
  IRosCoreTelemetry,
  IRosGeometryMsgsPose,
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
  telemetryStore: IRosCoreTelemetry;

  // Camera Stores
  camerasStore: IRosCameraMsgsCameras;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Science Bifrost states
  nirStore: IRosCoreNirProbeData;
  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
}
