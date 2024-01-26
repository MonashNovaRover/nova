import BifrostStatusStore from "./store/BifrostStatusStore";
import {createBifrostStore} from "./store/createBifrostStore";
import {uiSlice} from "./slices/UIReducer";
import {RosTopics} from "../ros/rosTopics";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),
  poseStore: createBifrostStore(RosTopics.POSE, {
    orientation: { x: 0, y: 0, z: 0, w: 0 },
    position: { x: 0, y: 0, z: 0 },
  }),
  driveStore: createBifrostStore(RosTopics.DRIVE_INFO, {
    drive_mode: 0,
    multiplier: 1,
    locked: false,
    autonomous_mode: false,
    connected: false,
    handbrake: false
  }),
  telemetryStore: createBifrostStore(RosTopics.TELEMETRTY, {
    wheels: [0,0,0,0].map(() => ({
      bus: "idk",
      id: 0,
      rotor_velocity: 0,
      q_current: 0,
      rotor_interval: 0,
      d_current: 0,
      resolver_position: 0,
      resolver_velocity: 0,
      power: 0,
      voltage: 0,
      temperature: 0
    })),
    pivots: [0,0,0,0].map(() => ({
      bus: "idk",
      id: 0,
      rotor_velocity: 0,
      q_current: 0,
      rotor_interval: 0,
      d_current: 0,
      resolver_position: 0,
      resolver_velocity: 0,
      power: 0,
      voltage: 0,
      temperature: 0
    })),
  })
};
