import BifrostStatusStore from "./store/BifrostStatusStore";
import {createBifrostStore} from "./store/createBifrostStore";
import {uiSlice} from "./slices/UIReducer";
import {RosTopics} from "../ros/rosTopics";
import {DriveMode} from "../ros/rosMessageTypes.ts";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),
  poseStore: createBifrostStore(RosTopics.POSE, {
    orientation: { x: 0, y: 0, z: 0, w: 0 },
    position: { x: 0, y: 0, z: 0 },
  }),
  driveStore: createBifrostStore(RosTopics.DRIVE_INFO, {
    drive_mode: DriveMode.TANK,
    multiplier: 1,
    locked: false,
    autonomous_mode: false,
    connected: false,
    handbrake: false
  }),
};
