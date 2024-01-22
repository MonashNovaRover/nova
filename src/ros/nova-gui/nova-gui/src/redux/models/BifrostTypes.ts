import { RosTopic } from "../../ros/topics/rosTopic";

export enum BifrostConnectionStatus {
  DISCONNECTED = "Disconnected",
  CONNECTING = "Connecting",
  CONNECTED = "Connected",
}

export interface BifrostStatus {
  connectionStatus: BifrostConnectionStatus;
  subscribedTopics: RosTopic[];
}

export const initalBifrostState: BifrostStatus = {
  connectionStatus: BifrostConnectionStatus.DISCONNECTED,
  subscribedTopics: [],
};
