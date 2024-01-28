import { IRosGeometryMsgsPose } from "../ros/rosMessageTypes";
import { BifrostStatus } from "./models/bifrost/BifrostTypes";
import { CameraStreamerState } from "./models/CameraStreamState";
import { UIState } from "./models/UIState";

export interface RootState {
  // Bifrost Stores
  bifrostStatus: BifrostStatus;
  poseStore: IRosGeometryMsgsPose;

  // Regular Stores
  uiState: UIState;
  cameraStreamerState: CameraStreamerState;
}
