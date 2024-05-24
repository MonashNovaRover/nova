import { useCallback, useEffect, useState } from "react";
import { RosAction } from "../../ros/actions/RosAction";
import { rosActionMessages } from "../../ros/actions/RosActionTypes";
import toast from "react-hot-toast";
import { RosActionInterface } from "../../ros/actions/RosActionMessages";
import useWebSocket, { ReadyState } from "react-use-websocket";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { v4 as uuidv4 } from "uuid";
import { WsActionMessage } from "./wsActionMessages";

/**
 * ! This Hook is just a very rudimentary attempt at getting a Ros Action Client Working
 * ! As of Now this exists as a standalone hook and doesn't update anything on Redux. Possibility of Backfiring is high in this case.
 * ! Multiple Ws Connections could become a problem (not a comms level problem, but a relatively serious one)
 * ! This should become a Part of Bifrost at some point, but due to time constraints wasn't done
 * ! Kabi Had 2 x 10% Assignments due that Night. Kabi is Sorry : (
 */

/**
 * Custom hook for using ROS action.
 * @param action - The ROS action to use.
 * @returns the sendGoal function, goalResponse, cancelGoal function, and feedback state
 */
export const useRosAction = (action: RosAction) => {
  const baseStationIP = useSelector(
    (state: RootState) => state.uiState.baseStationIP
  );

  const { sendJsonMessage, lastJsonMessage, readyState } =
    useWebSocket<WsActionMessage>("ws://" + baseStationIP + ":9090");

  const [feedback, setFeedback] =
    useState<RosActionInterface[typeof action]["feedback"]>();

  const [goalResponse, setGoalResponse] =
    useState<RosActionInterface[typeof action]["goalResponse"]>();

  const [currentGoalId, setCurrentGoalId] = useState<string>();

  /**
   * Sends a goal to the ROS action.
   * @param goal - The goal message to send.
   */
  const sendGoal = useCallback(
    (goal: RosActionInterface[typeof action]["goal"]) => {
      if (readyState !== ReadyState.OPEN)
        toast.error(`Unable to Send Goal on Action: ${action}`);
      if (!goal) {
        toast.error(`Goal is Undefined on Action: ${action}`);
        return;
      }

      setGoalResponse(undefined);

      const goalId = uuidv4();
      const goalArray = Object.values(goal);

      const jsonMessage = {
        op: "send_action_goal",
        id: goalId,
        action: action.toString(),
        action_type: rosActionMessages[action],
        args: goalArray,
        feedback: true,
      };

      sendJsonMessage(jsonMessage);
      setCurrentGoalId(goalId);

    },
    [action, readyState, sendJsonMessage]
  );

  /**
   * Cancels all goals on the action server.
   */
  const cancelGoal = useCallback(() => {
    if (readyState !== ReadyState.OPEN || !currentGoalId)
      toast.error(`Unable to Cancel Goal on Action: ${action}`);
  
    const jsonMessage = {
      op: "cancel_action_goal",
      id: currentGoalId,
      action: action.toString(),
    };

    sendJsonMessage(jsonMessage);

    setCurrentGoalId(undefined);
    setFeedback(undefined);
    setGoalResponse(undefined);
  }, [action, currentGoalId, readyState, sendJsonMessage]);

  /**
   * Handles All Messages from rosbridge_server according to the Rosbridge Protocol[https://github.com/RobotWebTools/rosbridge_suite/blob/ros2/ROSBRIDGE_PROTOCOL.md]
   */
  type ActionResponse = {
    status: number;
    result: unknown; 
  };

  useEffect(() => {
    if (!lastJsonMessage) return;
    switch (lastJsonMessage.op) {
      case "action_feedback": {
        if (lastJsonMessage.id === currentGoalId)
          setFeedback(
            lastJsonMessage.values as unknown as RosActionInterface[typeof action]["feedback"]
          );
        break;
      }
      case "action_result": {
        if (lastJsonMessage.id === currentGoalId) {
          setGoalResponse(
            (lastJsonMessage.values as ActionResponse).result as RosActionInterface[typeof action]["goalResponse"]
          );
        }
        break;
      }
      default:
        break;
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastJsonMessage]);

  return { sendGoal, goalResponse, cancelGoal, feedback };
};