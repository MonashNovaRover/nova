import BifrostStatusStore from "./store/bifrost/BifrostStatusStore";
import { uiSlice } from "./slices/UIReducer";
import { createBifrostStore } from "./store/bifrost/createBifrostStore";
import { RosService } from "../ros/services/rosService";
import { RosTopic } from "../ros/topics/rosTopic";

export const rootReducer = {
  uiState: uiSlice.reducer,
  bifrostStatus: BifrostStatusStore(),

  /*ipList: createBifrostStore({ service: RosService.GET_IP_LIST }, { ips: [] }),*/
  ramanSpecServiceStore: createBifrostStore(
    { service: RosService.CALL_RAMAN_SPEC},
    { continuousendedsignal: false,
      isvalid: false,
      spectra: [0]}
  ),
  poseStore: createBifrostStore(
    { topic: RosTopic.POSE },
    {
      orientation: { x: 0, y: 0, z: 0, w: 0 },
      position: { x: 0, y: 0, z: 0 },
    }
  ),
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
};
