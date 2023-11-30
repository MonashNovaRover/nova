import { Ros, Topic } from "roslib";
import { BifrostConnectionStatus } from "../models/BifrostTypes";
import { RootState } from "../RootState";
import { RosTopicInterfaces } from "../../ros/rosTopicInterfaces";
import { RosTopics } from "../../ros/rosTopics";
import { rosMessages } from "../../ros/rosMessages";

export enum BifrostActionTypes {
  UPDATE_DATA = "UPDATE_DATA_",
  INITIATE_CONTACT = "INITIATE_CONTACT",
  CONNECTION_UPDATE = "CONNECTION_UPDATE",
  SUBSCRIBE_TOPIC = "SUBSCRIBE_TOPIC",
}

export interface BifrostActionType<P> {
  type: string;
  payload: P;
}

export function createBifrostAction(topic: RosTopics, ros?: Ros) {
  return {
    _update(
      object: RosTopicInterfaces[typeof topic]
    ): () => BifrostActionType<RosTopicInterfaces[typeof topic]> {
      return () => ({
        type: BifrostActionTypes.UPDATE_DATA.toString() + topic.toString(),
        payload: { ...object },
      });
    },
    _updateBifrostConnectionStatus(connectionStatus: BifrostConnectionStatus) {
      return () => ({
        type: BifrostActionTypes.CONNECTION_UPDATE,
        payload: connectionStatus,
      });
    },
    _updateSubscribedTopics(topic: RosTopics) {
      return () => ({
        type: BifrostActionTypes.SUBSCRIBE_TOPIC,
        payload: topic,
      });
    },
    _updateState(object: RosTopicInterfaces[typeof topic]) {
      return (dispatch: Function) => {
        dispatch(this._update(object as RosTopicInterfaces[typeof topic]));
      };
    },
    updateBifrostConnection(connectionStatus: BifrostConnectionStatus) {
      return (dispatch: Function) => {
        dispatch(this._updateBifrostConnectionStatus(connectionStatus));
      };
    },
    syncWithRover() {
      return (dispatch: Function, getState: () => RootState) => {
        const state = getState();
        if (state.bifrostStatus.subscribedTopics.includes(topic) || !ros)
          return;

        const rosTopic = new Topic({
          ros: ros,
          name: topic.toString(),
          messageType: rosMessages[topic],
        });

        rosTopic.subscribe(() => {});

        rosTopic.on("message", (message: RosTopicInterfaces[typeof topic]) => {
          this._updateState(message);
        });

        dispatch(this._updateSubscribedTopics(topic));
      };
    },
  };
}
