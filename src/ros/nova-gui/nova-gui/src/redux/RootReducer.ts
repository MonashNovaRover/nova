import BifrostStatusStore from "./store/bifrost/BifrostStatusStore";
import { createBifrostStore } from "./store/bifrost/createBifrostStore";
import { RosService } from "../ros/services/rosService";
import { RosTopic } from "../ros/topics/rosTopic";
import { uiSlice } from "./slices/UISlice";
import { cameraStreamerSlice } from "./slices/CameraStreamSlice";
import { IRosSensorMsgsRange } from "../ros/rosTypes";

export const rootReducer = {
  // Bifrost Stores
  bifrostStatus: BifrostStatusStore(),

  poseStore: createBifrostStore(
    { topic: RosTopic.POSE },
    {
      orientation: { x: 0, y: 0, z: 0, w: 0 },
      position: { x: 0, y: 0, z: 0 },
    }
  ),

  // Drive Reducers

  driveStore: createBifrostStore(
    { topic: RosTopic.DRIVE_INFO },
    {
      drive_mode: 0,
      multiplier: 1,
      locked: false,
      autonomous_mode: false,
      connected: false,
      handbrake: false,
    }
  ),
  telemetryStore: createBifrostStore(
    { topic: RosTopic.TELEMETRY },
    {
      wheels: [0, 0, 0, 0].map(() => ({
        bus: "",
        id: 0,
        rotor_velocity: 0,
        q_current: 0,
        rotor_interval: 0,
        d_current: 0,
        resolver_position: 0,
        resolver_velocity: 0,
        power: 0,
        voltage: 0,
        temperature: 0,
      })),
      pivots: [0, 0, 0, 0].map(() => ({
        bus: "",
        id: 0,
        rotor_velocity: 0,
        q_current: 0,
        rotor_interval: 0,
        d_current: 0,
        resolver_position: 0,
        resolver_velocity: 0,
        power: 0,
        voltage: 0,
        temperature: 0,
      })),
    }
  ),

  // Cameras2 Reducers
  camerasStore: createBifrostStore(
    { topic: RosTopic.CAMERAS },
    { cameras: [] }
  ),
  ipList: createBifrostStore({ service: RosService.GET_IP_LIST }, { ips: [] }),

  // Science Reducers
  tofStore: createBifrostStore(
    { topic: RosTopic.TOF },
    {
      header: {
        frame_id: ""
      } as IRosSensorMsgsRange["header"],
      min_range: 0.0,
      max_range: 150.0,
      range: 0.0
    } as IRosSensorMsgsRange),

  // Regular Reducers
  uiState: uiSlice.reducer,
  cameraStreamerState: cameraStreamerSlice.reducer,
};
