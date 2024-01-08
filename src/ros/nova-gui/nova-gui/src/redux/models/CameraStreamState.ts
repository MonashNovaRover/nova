export interface Camera {
  serial: string;
  peerId: string;
}

export enum CameraStreamerStatus {
  DISCONNECTED = "Disconnected",
  CONNECTING = "Connecting",
  CONNECTED = "Connected",
}

export interface CameraStreamerState {
  cameras: Camera[];
  status: CameraStreamerStatus;
}

export const initialCameraStreamerState: CameraStreamerState = {
  cameras: [],
  status: CameraStreamerStatus.DISCONNECTED,
};
