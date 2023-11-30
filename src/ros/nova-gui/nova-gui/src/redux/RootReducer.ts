import BifrostStatusStore from "./store/BifrostStatusStore";
import { createBifrostStore } from "./store/createBifrostStore";
import { uiSlice } from "./slices/UIReducer";
import { RosTopics } from "../ros/rosTopics";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),
  demoStore: createBifrostStore(RosTopics.DEMO_TOPIC, { data: "" }),
  poseStore: createBifrostStore(RosTopics.POSE, {
    orientation: { x: 0, y: 0, z: 0, w: 0 },
    position: { x: 0, y: 0, z: 0 },
  }),
};
