import { IRosNovaInterfacesStepperActionFeedback, IRosNovaInterfacesStepperActionGoal, IRosNovaInterfacesStepperActionResult } from "../rosTypes";
import { RosAction } from "./RosAction";

export interface RosActionInterface {
  [RosAction.NULL_ACTION]: {
    goal: undefined;
    goalResponse: undefined;
    feedback: undefined;
  };
  [RosAction.SCIENCE_SAMPLE_TRAY]: {
    goal: IRosNovaInterfacesStepperActionGoal;
    goalResponse: IRosNovaInterfacesStepperActionResult;
    feedback: IRosNovaInterfacesStepperActionFeedback;
  };
}
