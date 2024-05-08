import { useCallback, useContext, useEffect, useState } from "react";
import { RosAction } from "../../ros/actions/RosAction";
import { RosContext } from "../../redux/context/RosContext";
import * as RosLib from "roslib";
import { rosActionMessages } from "../../ros/actions/RosActionTypes";
import toast from "react-hot-toast";
import { RosActionInterface } from "../../ros/actions/RosActionMessages";

/**
 * ! This Hook is just a very rudimentary attempt at getting a Ros Action Client Working
 * ! As of Now this exists as a standalone hook and doesn't update anything on Redux. Possibility of Backfiring is high in this case.
 * ! This should become a Part of Bifrost at some point, but due to time constraints wasn't done
 * ! Kabi is Sorry : (
 */

/**
 * Custom hook for using ROS action.
 * @param action - The ROS action to use.
 * @returns the sendGoal function, cancelGoal function, and feedback state
 */
export const useRosAction = (action: RosAction) => {
  const ros = useContext(RosContext);

  const [actionClient, setActionClient] = useState<RosLib.ActionClient>();
  const [feedback, setFeedback] =
    useState<RosActionInterface[typeof action]["feedback"]>();

  useEffect(() => {
    if (ros) {
      const actionClient = new RosLib.ActionClient({
        ros: ros,
        actionName: rosActionMessages[action],
        serverName: action as string,
      });
      setActionClient(actionClient);
    }
  }, [setActionClient, action, ros]);

  /**
   * Sends a goal to the ROS action.
   * @param goal - The goal message to send.
   */
  const sendGoal = useCallback(
    (goal: RosActionInterface[typeof action]) => {
      if (!actionClient)
        toast.error(`Unable to Send Goal to Action: ${action}`);

      const actionGoal = new RosLib.Goal({
        actionClient: actionClient!,
        goalMessage: goal,
      });

      actionGoal.send();
      actionGoal.on("feedback", (feedback) => setFeedback(feedback));
      actionGoal.on("status", (status) =>
        console.log(`Status of Action :${status}`)
      );
      actionGoal.on("result", (event) => console.log(event));
    },
    [action, actionClient]
  );

  /**
   * Cancels all goals on the action server.
   */
  const cancelGoal = useCallback(
    (goal: RosActionInterface[typeof action]["goal"]) => {
      if (!actionClient)
        toast.error(`Unable to Cancel Goal to Action: ${action}`);

      const actionGoal = new RosLib.Goal({
        actionClient: actionClient!,
        goalMessage: goal,
      });

      actionGoal.send();
    },
    [action, actionClient]
  );

  return { sendGoal, cancelGoal, feedback };
};
