import { Reducer } from "@reduxjs/toolkit";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../actions/createBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { RosTopicInterfaces } from "../../ros/topics/rosTopicTypes";
import { BifrostProps } from "../actions/useBifrostAction";
import { RosService } from "../../ros/services/rosService";
import { RosServiceInterface } from "../../ros/services/rosServiceTypes";

export const createCustomReducer = <S>(initialState: S, handlers: any) => {
  const reducer = (state: S = initialState, action: any): S => {
    if (handlers.hasOwnProperty(action.type)) {
      return handlers[action.type](state, action);
    } else {
      return state;
    }
  };
  return reducer as Reducer<S>;
};

// Apologies: I tried my best to get Inital State to be it's type, but my brain is too tiny to
// get it to work :(. `any` should do it for now.
export const createBifrostStore = (props: BifrostProps, initialState: any) => {
  const { service = RosService.NULL_SERVICE, topic = RosTopic.NULL_TOPIC } =
    props;

  const reducerFunctions: any = {};
  if (topic !== RosTopic.NULL_TOPIC) {
    reducerFunctions[BifrostActionTypes.UPDATE_TOPIC_STATE + topic] = (
      _: RosTopicInterfaces[typeof topic],
      action: BifrostActionType<RosTopicInterfaces[typeof topic]>
    ) => {
      return {
        ...action.payload,
      };
    };
  }

  if (service !== RosService.NULL_SERVICE) {
    reducerFunctions[BifrostActionTypes.UPDATE_SERVICE_STATE + "/" + service] =
      (
        _: RosServiceInterface[typeof service],
        action: BifrostActionType<RosServiceInterface[typeof service]>
      ) => {
        return {
          ...action.payload,
        };
      };
  }

  return createCustomReducer<RosTopicInterfaces[typeof topic]>(
    initialState,
    reducerFunctions
  );
};
