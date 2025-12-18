import { IRosScienceInterfacesPumpsActionFeedback, IRosScienceInterfacesPumpsActionGoal, IRosScienceInterfacesPumpsActionResult, IRosScienceInterfacesStepperActionFeedback, IRosScienceInterfacesStepperActionGoal, IRosScienceInterfacesStepperActionResult } from "../rosTypes";
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
    goal: IRosScienceInterfacesStepperActionGoal;
    goalResponse: IRosScienceInterfacesStepperActionResult;
    feedback: IRosScienceInterfacesStepperActionFeedback;
  };
  [RosAction.CAROUSEL_ACTION]: {
    goal: IRosScienceInterfacesStepperActionGoal;
    goalResponse: IRosScienceInterfacesStepperActionResult;
    feedback: IRosScienceInterfacesStepperActionFeedback;
  };
  [RosAction.PUMPS]: {
    goal: IRosScienceInterfacesPumpsActionGoal;
    goalResponse: IRosScienceInterfacesPumpsActionResult;
    feedback: IRosScienceInterfacesPumpsActionFeedback;
  };
}
