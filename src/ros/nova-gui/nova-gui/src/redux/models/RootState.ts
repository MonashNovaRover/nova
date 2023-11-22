import { IRadioStatus, ITalker } from "../../ros/types";
import { BifrostStatus } from "./BifrostTypes";
import { UIState } from "./UIState";

export interface RootState {
  uiState: UIState;
  bifrostStatus: BifrostStatus;
  radioState?: IRadioStatus;
  talkerState?: ITalker;
}
