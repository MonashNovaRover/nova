import BifrostStatusStore from "./store/bifrost/BifrostStatusStore";
import {createBifrostStore} from "./store/bifrost/createBifrostStore";
import {RosService} from "../ros/services/rosService";
import {RosTopic} from "../ros/topics/rosTopic";
import {
  IRosCmdInterfacesCmdFeedback,
  IRosCmdInterfacesCmDsFeedback,
  IRosNovaInterfacesNirProbeDataConst,
  IRosSensorMsgsRange,
} from "../ros/rosTypes";
import {uiSlice} from "./slices/UISlice";
import {cameraStreamerSlice} from "./slices/CameraStreamSlice";
import {BLCMD_INDEX} from "../constants";


export const rootReducer = {
  // Bifrost Stores
  bifrostStatus: BifrostStatusStore(),

  // Drive Reducers
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
  driveTelemetryStore: createBifrostStore(
    { topic: RosTopic.DRIVE_TELEMETRY },
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

  // Arm Reducers
  armTelemetryStore: createBifrostStore({ topic: RosTopic.ARM_TELEMETRY }, {
    arm_motors: [0, 0, 0, 0, 0, 0].map(
      () =>
        ({
          current: 0,
        }) as IRosCmdInterfacesCmdFeedback
    ),
  } as IRosCmdInterfacesCmDsFeedback),
  rfidDataStore: createBifrostStore(
    { topic: RosTopic.RFID_DATA },
    {
      data: "",
    }
  ),

  // Cameras2 Reducers
  camerasStore: createBifrostStore(
    { topic: RosTopic.CAMERAS },
    { cameras: [] }
  ),
  ipList: createBifrostStore(
    { service: RosService.GET_IP_LIST }, 
    { ips: [] }
  ),

  blcmdStatusStore: createBifrostStore(
    { topic: RosTopic.BLCMD_ERRORS },
    {
      blcmds: Object.keys(BLCMD_INDEX).map((id) => ({
        id,
        gate_fault: false,
        stall_fault: false,
        resolver_fault: false,
        overspeed_fault: false,
      })),
    }
  ),
  
  // Science Reduceers
  kilnData: createBifrostStore(
    { topic: RosTopic.KILN_DATA },
    {
      temp: [0, 0, 0], // current converted temp readings [C]
      state: false, // current status of Kiln: True if On
    }
  ),
  kilnCommand: createBifrostStore(
    { service: RosService.KILN_COMMAND },
    {
      success: true, // whether the last service request succeeded or not: False will show error on Toggle Kiln Button
    }
  ),
  tofStore: createBifrostStore(
    { topic: RosTopic.TOF },
    {
      header: {
        frame_id: ""
      } as IRosSensorMsgsRange["header"],
      min_range: 0.0,
      max_range: 150.0,
      range: 0.0
    } as IRosSensorMsgsRange
  ),
  nirStore: createBifrostStore(
    { topic: RosTopic.NIR_DATA },
    {
      data: 0,
      led: IRosNovaInterfacesNirProbeDataConst.LED_OFF,
    }
  ),
  microscopeServoStore: createBifrostStore(
    { topic: RosTopic.MICROSCOPE_SERVO },
    { angle: 45 }
  ),
  microscopeServiceStore: createBifrostStore(
    { service: RosService.MOVE_MICROSCOPE_SERVO },
    { success: true }
  ),
  ramanSpecServiceStore: createBifrostStore(
    { service: RosService.CALL_RAMAN_SPEC},
    { continuousendedsignal: false }
  ),
  ramanSpecMessageStore: createBifrostStore(
    { topic: RosTopic.RAMAN_SPEC_MSG },
    {
      isvalid: true,
      spectrum: [1]
    }
  ),
  uvVisSpecStore: createBifrostStore(
    { topic: RosTopic.UV_VIS_SPEC },
    { luminance: [0,0,0] }
  ),

  // Regular Stores
  uiState: uiSlice.reducer,
  cameraStreamerState: cameraStreamerSlice.reducer,
};
