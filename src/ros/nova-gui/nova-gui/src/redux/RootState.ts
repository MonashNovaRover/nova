import { IRosBLCMDStatusMessage, IRosDemoMessage } from "../ros/rosMessageTypes";
import { BifrostStatus } from "./models/BifrostTypes";
import { UIState } from "./models/UIState";

export interface RootState {
  uiState: UIState;
  bifrostStatus: BifrostStatus;
  demo: IRosDemoMessage;
  blcmdStore: IRosBLCMDStatusMessage;
}
