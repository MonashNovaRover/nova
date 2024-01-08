import BifrostStatusStore from "./store/bifrost/BifrostStatusStore";
import { createBifrostStore } from "./store/bifrost/createBifrostStore";
import { uiSlice } from "./slices/UISlice";
import { RosTopics } from "../ros/rosTopics";
import { cameraStreamerSlice } from "./slices/CameraStreamSlice";

export const rootReducer = {
  // Bifrost Stores
  bifrostStatus: BifrostStatusStore(),
  poseStore: createBifrostStore(RosTopics.POSE, {
    orientation: { x: 0, y: 0, z: 0, w: 0 },
    position: { x: 0, y: 0, z: 0 },
  }),

  // Regular Stores
  uiState: uiSlice.reducer,
  cameraStreamerState: cameraStreamerSlice.reducer,
};
