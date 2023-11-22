import {
  BifrostActionType,
  BifrostActionTypes,
} from "../actions/createBifrostAction";
import {
  BifrostConnectionStatus,
  BifrostStatus,
  initalBifrostState,
} from "../models/BifrostTypes";
import { createCustomReducer } from "./createBifrostStore";

export default function BifrostStatusStore() {
  const reducerFunctions = {
    [BifrostActionTypes.ESTABLISH_CONTACT]: (
      state: BifrostStatus,
      action: BifrostActionType<BifrostConnectionStatus>
    ) => {
      action;
      return {
        ...state,
        connectionStatus: action.payload,
      };
    },
  };

  return createCustomReducer<BifrostStatus>(
    initalBifrostState,
    reducerFunctions
  );
}
