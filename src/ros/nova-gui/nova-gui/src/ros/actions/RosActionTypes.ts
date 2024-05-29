import { RosAction } from "./RosAction";

export const rosActionMessages = {
  [RosAction.NULL_ACTION]: "",
  [RosAction.FIBONACCI]: "example_interfaces/Fibonacci",
  [RosAction.SAMPLE_TRAY]: "nova_interfaces/action/Stepper",
  [RosAction.CAROUSEL_ACTION]: "nova_interfaces/actions/Stepper",
  [RosAction.PUMPS]: "nova_interfaces/actions/Pumps",
};
