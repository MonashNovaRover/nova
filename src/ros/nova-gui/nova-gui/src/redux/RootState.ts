import {
  IRosCameraMsgsGetIpListResponse,
  IRosCoreDriveInfo,
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
  driveStore: IRosCoreDriveInfo;
  telemetryStore: IRosCoreTelemetry;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
}
