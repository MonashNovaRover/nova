import { useCallback, useEffect, useRef, useState } from "react";
import { PeerMessage, ServerMessage } from "./serverMessages";
import useWebSocket from "react-use-websocket";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import toast from "react-hot-toast";

export enum StreamingState {
  STOPPED,
  LOADING,
  STREAMING,
}

const ICE_SERVERS = [
  {
    //! This only works when the base station is connected to the internet !
    //! Gotta Add the STUN Server spin up by cameras2. Or could be possibly fixed up by Novafox
    urls: [
      "stun:stun.l.google.com:19302",
      "stun:stun1.l.google.com:19302",
      "stun:stun.iptel.org",
    ],
  },
];

export const useCameraStream = (
  cameraSerial: string,
  videoRef: React.MutableRefObject<HTMLVideoElement | null>
) => {
  const [isWsOpen, setWsOpen] = useState(false);

  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    "ws://192.168.1.204:8443",
    {
      onOpen: () => {
        setWsOpen(true);
      },
    }
  );

  const camerasFromRos = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  const isCameraOnline = camerasFromRos
    .map((cam) => cam.serial)
    .includes(cameraSerial);

  const [sessionId, setSessionId] = useState<string>();
  const rtcRef = useRef<RTCPeerConnection>();
  const peerId = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras[cameraSerial]
  );

  const [streamingState, setStreamingState] = useState<StreamingState>(
    StreamingState.STOPPED
  );

  const sendSessionStartMessage = useCallback(() => {
    if (!isWsOpen || !peerId) {
      toast.error(`${cameraSerial} unable to start up`);
      return;
    }
    setStreamingState(StreamingState.LOADING);
    sendJsonMessage({ type: "startSession", peerId: peerId });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [sendJsonMessage, peerId, isWsOpen]);

  const iceCandidateCallback = useCallback(
    (event: RTCPeerConnectionIceEvent) => {
      if (!event.candidate) return;
      if (!sessionId) return;
      sendJsonMessage({
        type: "peer",
        sessionId: sessionId,
        ice: event.candidate.toJSON(),
      });
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [sessionId]
  );

  const handlePeerMessage = useCallback(
    async (rtcPeerConnection: RTCPeerConnection, message: PeerMessage) => {
      if (message.sdp) {
        rtcPeerConnection.setRemoteDescription(message.sdp);
        const answer = await rtcPeerConnection.createAnswer();
        await rtcPeerConnection.setLocalDescription(answer);
        if (!rtcPeerConnection.localDescription || !sessionId) return;
        sendJsonMessage({
          type: "peer",
          sessionId: sessionId,
          sdp: rtcPeerConnection.localDescription.toJSON(),
        });
      } else if (message.ice) {
        const candidate = new RTCIceCandidate(message.ice);
        await rtcPeerConnection.addIceCandidate(candidate);
      } else {
        throw new Error(`Unknown peer message: ${message}`);
      }
    },
    [sendJsonMessage, sessionId]
  );

  const handOverRTCPeerConnection: () => RTCPeerConnection = useCallback(() => {
    if (rtcRef.current) {
      return rtcRef.current;
    } else {
      const rtcConnection = new RTCPeerConnection({
        iceServers: ICE_SERVERS,
      });

      rtcConnection.onicecandidate = iceCandidateCallback;
      rtcConnection.ontrack = (event) => {
        setStreamingState(StreamingState.STREAMING);
        if (videoRef.current) videoRef.current.srcObject = event.streams[0];
      };

      rtcRef.current = rtcConnection;
      return rtcRef.current;
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [videoRef]);

  useEffect(() => {
    if (!lastJsonMessage) return;
    switch (lastJsonMessage.type) {
      case "welcome":
        break;
      case "registered": {
        sendSessionStartMessage();
        break;
      }
      case "sessionStarted": {
        setSessionId(lastJsonMessage.sessionId);
        break;
      }
      case "peer": {
        const rtcPeerConnection = handOverRTCPeerConnection();
        handlePeerMessage(rtcPeerConnection, lastJsonMessage);
        break;
      }
      default:
        throw new Error(`Unknown message ${lastJsonMessage}`);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastJsonMessage]);

  return { streamingState, sendSessionStartMessage, isCameraOnline };
};
