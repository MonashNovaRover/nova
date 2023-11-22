import { Ros } from "roslib";
import { BifrostConnectionStatus } from "../models/BifrostTypes";
import { RootState } from "../models/RootState";
import { RosTopics, RosTopicTypes } from "../../ros/topics";

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
    update(
      object: RosTopicTypes[typeof topic]
    ): () => BifrostActionType<RosTopicTypes[typeof topic]> {
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
    updateStuff(object: RosTopicTypes[typeof topic]) {
      return async (dispatch: Function) => {
        dispatch(this.update(object));
      };
    },
    initiateContact(rosUrl: string) {
      return async (dispatch: Function) => {
        dispatch(
          this._updateBifrostConnectionStatus(
            BifrostConnectionStatus.CONNECTING
          )
        );

        const ros = new Ros({ url: rosUrl });

        ros.on("connection", () => {
          dispatch(
            this._updateBifrostConnectionStatus(
              BifrostConnectionStatus.CONNECTED
            )
          );
        });

        ros.on("error", () => {
          dispatch(
            this._updateBifrostConnectionStatus(
              BifrostConnectionStatus.DISCONNECTED
            )
          );
        });

        ros.on("close", () => {
          dispatch(
            this._updateBifrostConnectionStatus(
              BifrostConnectionStatus.DISCONNECTED
            )
          );
        });
      };
    },
  };
}
