import { IRosNovaInterfacesPumpsActionFeedback, IRosNovaInterfacesPumpsActionGoal, IRosNovaInterfacesPumpsActionResult, IRosNovaInterfacesStepperActionFeedback, IRosNovaInterfacesStepperActionGoal, IRosNovaInterfacesStepperActionResult } from "../rosTypes";
import { RosAction } from "./RosAction";

export interface RosActionInterface {
  [RosAction.NULL_ACTION]: {
    goal: undefined;
    goalResponse: undefined;
    feedback: undefined;
  };
  [RosAction.FIBONACCI]: {
    goal: {
      order: number;
    };
    goalResponse: {
      sequence: number[];
    };
    feedback: {
      sequence: number[];
    };
  };
  [RosAction.SAMPLE_TRAY]: {
    goal: IRosNovaInterfacesStepperActionGoal;
    goalResponse: IRosNovaInterfacesStepperActionResult;
    feedback: IRosNovaInterfacesStepperActionFeedback;
  };
  [RosAction.CAROUSEL_ACTION]: {
    goal: IRosNovaInterfacesStepperActionGoal;
    goalResponse: IRosNovaInterfacesStepperActionResult;
    feedback: IRosNovaInterfacesStepperActionFeedback;
  };
  [RosAction.PUMPS]: {
    goal: IRosNovaInterfacesPumpsActionGoal;
    goalResponse: IRosNovaInterfacesPumpsActionResult;
    feedback: IRosNovaInterfacesPumpsActionFeedback;
  };
}
