import { RosTopics } from "../../ros/topics";
import BifrostStatusStore from "../store/BifrostStatusStore";
import { createBifrostStore } from "../store/createBifrostStore";
import { uiSlice } from "./UIReducer";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),
  radioState: createBifrostStore(RosTopics.RADIO_STATUS, { ping: 900 }),
  talkerStore: createBifrostStore(RosTopics.TALKER, { data: "" }),
};
