import { RosAction } from "./RosAction";

export interface RosActionInterface {
  [RosAction.NULL_ACTION]: {
    goal: undefined;
    goalResponse: undefined;
    feedback: undefined;
  };
}
