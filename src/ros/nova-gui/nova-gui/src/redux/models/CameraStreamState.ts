export enum CameraStreamerStatus {
  DISCONNECTED = "Disconnected",
  CONNECTING = "Connecting",
  CONNECTED = "Connected",
}

export interface CameraStreamerState {
  status: CameraStreamerStatus;
  cameras: { [serial: string]: string }; // {serial : peerId}
}

export const initialCameraStreamerState: CameraStreamerState = {
  cameras: {},
  status: CameraStreamerStatus.DISCONNECTED,
};
