import {DependencyList, useCallback, useContext, useEffect, useRef} from "react";
import {RosContext} from "../../redux/context/ros/RosContext.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {Topic} from "roslib";
import {rosTopicMessages} from "../../ros/topics/rosTopicMessages.ts";
import {RosTopicInterfaces} from "../../ros/topics/rosTopicTypes.ts";

/**
 * Simple hook for adding a callback for some ros topic subscription. Do not vary the topic.
 * @param topic The ROS topic to use
 * @param callback The callback function for when a new message is received
 * @param extraDeps Dependencies for the callback function
 */
export const useRosSubscription = <T>(
  topic: RosTopic, callback: (message: T & RosTopicInterfaces[typeof topic]) => void, extraDeps?: DependencyList
) => {
  const ros = useContext(RosContext);
  const rosTopicRef = useRef<Topic | undefined>(undefined);

  const cachedCallback = useCallback(callback, extraDeps ?? [callback]) // eslint-disable-line react-hooks/exhaustive-deps

  useEffect(() => {
    if (ros == undefined)
      return;

    // Initializes rosTopicRef
    if (rosTopicRef.current === undefined) {
      rosTopicRef.current = new Topic({
        ros: ros,
        name: topic.toString(),
        messageType: rosTopicMessages[topic],
      });
    }

    const subscriptionCallback = () => {};

    rosTopicRef.current.subscribe(subscriptionCallback);
    rosTopicRef.current.on("message", cachedCallback);

    return () => {
      rosTopicRef.current?.unsubscribe(subscriptionCallback);
      rosTopicRef.current?.removeListener("message", cachedCallback);
    };
  }, [ros, cachedCallback, topic]);

  // Clean up at end
  useEffect(() => {
    return () => {
      rosTopicRef.current = undefined
    };
  }, [ros]);
}
