import {
  IRosActionMsgsCancelGoalResponse,
  IRosActionMsgsGoalInfo,
} from "../rosTypes";
import { RosAction } from "./RosAction";

export interface RosActionInterface {
  [RosAction.NULL_ACTION]: {
    goal: undefined;
    goalResponse: undefined;
    feedback: undefined;
  };

  // ! Dummy Stuff: Doesnn't make sense but are there solely as props (not react props, actual props!)
  [RosAction.AUTONOMOUS_ACTION]: {
    goal: IRosActionMsgsGoalInfo;
    goalResponse: IRosActionMsgsCancelGoalResponse;
    feedback: IRosActionMsgsGoalInfo;
  };
}
