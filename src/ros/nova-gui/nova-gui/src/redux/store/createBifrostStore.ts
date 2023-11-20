import { Reducer } from "@reduxjs/toolkit";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../actions/createBifrostAction";
import { CornerDownLeft } from "react-feather";

const createCustomReducer = <S>(initialState: S, handlers: any) => {
  const reducer = (state: S = initialState, action: any): S => {
    if (handlers.hasOwnProperty(action.type)) {
      return handlers[action.type](state, action);
    } else {
      return state;
    }
  };
  return reducer as Reducer<S>;
};

export const createBifrostStore = <T>(initialState: T) => {
  const reducerFunctions = {
    [BifrostActionTypes.UPDATE_DATA]: (_: T, action: BifrostActionType<T>) => {
      action;
      return {
        ...action.payload,
      };
    },
  };

  return createCustomReducer<T>(initialState, reducerFunctions);
};
