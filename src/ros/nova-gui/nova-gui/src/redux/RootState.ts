import {
  IRosCameraMsgsGetIpListResponse,
  IRosCoreDriveInfo, IRosCoreNirData,
  IRosCoreTelemetry,
  IRosGeometryMsgsPose,
} from "../ros/rosTypes";
import { BifrostStatus } from "./models/bifrost/BifrostTypes";
import { UIState } from "./models/UIState";

export interface RootState {
  // Essential UI Management States
  uiState: UIState;
  bifrostStatus: BifrostStatus;

  // Bifrost States. Ros Stores go here
  poseStore: IRosGeometryMsgsPose;
  driveStore: IRosCoreDriveInfo;
  telemetryStore: IRosCoreTelemetry;
  ipList: IRosCameraMsgsGetIpListResponse;

  // Science Bifrost states
  nirStore: IRosCoreNirData;
}
