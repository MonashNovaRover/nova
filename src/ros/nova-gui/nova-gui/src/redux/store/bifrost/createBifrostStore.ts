import { Reducer } from "@reduxjs/toolkit";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../../actions/bifrost/createBifrostAction";
import { RosTopics } from "../../../ros/rosTopics";
import { RosTopicInterfaces } from "../../../ros/rosTopicTypes";

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
  initialState: RosTopicInterfaces[typeof topic]
) => {
  const reducerFunctions = {
    [BifrostActionTypes.UPDATE_DATA + topic]: (
      _: RosTopicInterfaces[typeof topic],
      action: BifrostActionType<RosTopicInterfaces[typeof topic]>
    ) => {
      action;
      return {
        ...action.payload,
      };
    },
  };

  return createCustomReducer<RosTopicInterfaces[typeof topic]>(
    initialState,
    reducerFunctions
  );
};
