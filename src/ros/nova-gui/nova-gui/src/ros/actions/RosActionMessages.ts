import { RosAction } from "./RosAction";

export interface RosActionInterface {
  [RosAction.NULL_ACTION]: {
    goal: undefined;
    goalResponse: undefined;
    feedback: undefined;
  };
  [RosAction.SCIENCE_SAMPLE_TRAY]: {
    goal: string;
    goalResponse: {
      success: boolean;
    };
    feedback: {
      current_position: number;
      goal_position: number;
    };
  };
}
