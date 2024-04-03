type PeerRole = "producer" | "listerner";

export interface SDPOffer {
  type: "offer";
  sdp: string;
}

export interface ICECandidate {
  candidate: string;
  sdpMLineIndex: number;
}

export interface CameraMetadata {
  serial: string;
}

export interface Producer {
  id: string;
  meta: CameraMetadata;
}

export interface WelcomeMessage {
  type: "welcome";
}

export interface RegisteredMessage {
  type: "registered";
}

export interface ErrorMessage {
  type: "error";
}

export interface ListMessage {
  type: "list";
  producers: Producer[];
}

export interface PeerStatusChangedMessage {
  type: "peerStatusChanged";
  roles: PeerRole[];
  peerId: string;
  meta?: CameraMetadata;
}

export interface StartSessionMessage {
  type: "sessionStarted";
  peer: string;
  sessionId: string;
}

export interface EndSessionMessage {
  type: "sessionStarted";
  peer: string;
  sessionId: string;
}

export interface PeerMessage {
  type: "peer";
  sessionId: string;
  sdp?: SDPOffer;
  ice?: ICECandidate;
}

export type ServerMessage =
  | WelcomeMessage
  | RegisteredMessage
  | ErrorMessage
  | ListMessage
  | PeerStatusChangedMessage
  | StartSessionMessage
  | EndSessionMessage
  | PeerMessage;
