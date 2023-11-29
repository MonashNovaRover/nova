import BifrostStatusStore from "./store/BifrostStatusStore";
import { createBifrostStore } from "./store/createBifrostStore";
import { uiSlice } from "./slices/UIReducer";
import { RosTopics } from "../ros/rosTopics";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),
  demoStore: createBifrostStore(RosTopics.DEMO_TOPIC, {}),
  blcmdStore: createBifrostStore(RosTopics.BLCMD_STATUS,{}),
};
