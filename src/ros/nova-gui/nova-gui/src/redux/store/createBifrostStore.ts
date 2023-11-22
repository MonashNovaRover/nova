import { Reducer } from "@reduxjs/toolkit";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../actions/createBifrostAction";
import { RosTopicTypes, RosTopics } from "../../ros/topics";

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

export const createBifrostStore = (
  topic: RosTopics,
  initialState: RosTopicTypes[typeof topic]
) => {
  const reducerFunctions = {
    [BifrostActionTypes.UPDATE_DATA + topic]: (
      _: RosTopicTypes[typeof topic],
      action: BifrostActionType<RosTopicTypes[typeof topic]>
    ) => {
      action;
      return {
        ...action.payload,
      };
    },
  };

  return createCustomReducer<RosTopicTypes[typeof topic]>(
    initialState,
    reducerFunctions
  );
};
