// These Types were listed as JSON Specs on https://github.com/RobotWebTools/rosbridge_suite/blob/ros2/ROSBRIDGE_PROTOCOL.md

export interface ActionFeedback {
  op: "action_feedback";
  id: string;
  action: string;
  values: object;
}

export interface ActionResult {
  op: "action_result";
  id: string;
  action: string;
  values: object;
  result: boolean;
}

export type WsActionMessage = ActionFeedback | ActionResult;
