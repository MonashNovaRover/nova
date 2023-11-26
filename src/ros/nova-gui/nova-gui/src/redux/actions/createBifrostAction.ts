import { Topic } from "roslib";
import { BifrostConnectionStatus } from "../models/BifrostTypes";
import {
  RosTopics,
  RosTopicInterfaces,
  rosTopicMessages,
} from "../../ros/topics";
import { RootState } from "../models/RootState";
import { useContext } from "react";
import { RosContext } from "../../RosRoot";

export enum BifrostActionTypes {
  UPDATE_DATA = "UPDATE_DATA_",
  INITIATE_CONTACT = "INITIATE_CONTACT",
  ESTABLISH_CONTACT = "ESTABLISH_CONTACT",
}

export interface BifrostActionType<P> {
  type: string;
  payload: P;
}

export function createBifrostAction(topic: RosTopics) {
  return {
    _update(
      object: RosTopicInterfaces[typeof topic]
    ): () => BifrostActionType<RosTopicInterfaces[typeof topic]> {
      return () => ({
        type: BifrostActionTypes.UPDATE_DATA.toString() + topic.toString(),
        payload: object,
      });
    },
    _updateBifrostConnectionStatus(connectionStatus: BifrostConnectionStatus) {
      return () => ({
        type: BifrostActionTypes.ESTABLISH_CONTACT,
        payload: connectionStatus,
      });
    },
    _updateState(object: RosTopicInterfaces[typeof topic]) {
      return async (dispatch: Function) => {
        dispatch(this._update(object));
      };
    },
    updateBifrostConnection(connectionStatus: BifrostConnectionStatus) {
      return (dispatch: Function) => {
        dispatch(this._updateBifrostConnectionStatus(connectionStatus));
      };
    },
    syncWithRover() {
      return (dispatch: Function, getState: () => RootState) => {
        const ros = useContext(RosContext);
        const state = getState();

        if (state.bifrostStatus.subsribedTopics.includes(topic) || !ros) return;

        const rosTopic = new Topic({
          ros: ros,
          name: topic.toString(),
          messageType: rosTopicMessages[topic],
        });

        rosTopic.on("message", (message: RosTopicInterfaces[typeof topic]) => {
          dispatch(this._updateState(message));
        });
      };
    },
  };
}
