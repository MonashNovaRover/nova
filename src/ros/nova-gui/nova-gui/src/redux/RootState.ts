import { IRosCoreDriveInfo, IRosGeometryMsgsPose, IRosCoreTelemetry } from "../ros/rosMessageTypes";
import { BifrostStatus } from "./models/BifrostTypes";
import { UIState } from "./models/UIState";

export interface RootState {
  // Essential UI Management States
  uiState: UIState;
  bifrostStatus: BifrostStatus;
  // Bifrost States. Ros Stores go here
  poseStore: IRosGeometryMsgsPose;
  driveStore: IRosCoreDriveInfo;
  telemetryStore: IRosCoreTelemetry;
}
