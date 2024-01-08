import { RosTopics } from "../../../ros/rosTopics";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../../actions/bifrost/createBifrostAction";
import {
  BifrostConnectionStatus,
  BifrostStatus,
  initalBifrostState,
} from "../../models/bifrost/BifrostTypes";
import { createCustomReducer } from "./createBifrostStore";

export default function BifrostStatusStore() {
  const reducerFunctions = {
    [BifrostActionTypes.CONNECTION_UPDATE]: (
      state: BifrostStatus,
      action: BifrostActionType<BifrostConnectionStatus>
    ) => {
      return {
        ...state,
        connectionStatus: action.payload,
      };
    },
    [BifrostActionTypes.SUBSCRIBE_TOPIC]: (
      state: BifrostStatus,
      action: BifrostActionType<RosTopics>
    ) => {
      return {
        ...state,
        subscribedTopics: [...state.subscribedTopics, action.payload],
      };
    },
  };

  return createCustomReducer<BifrostStatus>(
    initalBifrostState,
    reducerFunctions
  );
}
