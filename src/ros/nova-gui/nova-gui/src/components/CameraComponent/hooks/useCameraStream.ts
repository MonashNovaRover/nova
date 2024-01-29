import { useCallback, useEffect, useRef, useState } from "react";
import { Camera } from "../../../redux/models/CameraStreamState";
import { PeerMessage, ServerMessage } from "./serverMessages";
import useWebSocket from "react-use-websocket";

export enum StreamingState {
  STOPPED,
  LOADING,
  STREAMING,
}

const ICE_SERVERS = [
  {
    // STUN should not be necessary, but Firefox does not play nicely
    // with LAN connections without it.
    // - https://groups.google.com/g/mozilla.dev.media/c/rQUhtfBNRgU
    // - https://bugzilla.mozilla.org/show_bug.cgi?id=1659672
    // A local server can be used, such as this one:
    // https://github.com/jselbie/stunserver
    urls: [
      "stun:stun.l.google.com:19302",
      "stun:stun1.l.google.com:19302",
      "stun:stun.iptel.org",
    ],
  },
];

export const useCameraStream = (
  camera: Camera,
  videoRef: React.MutableRefObject<HTMLVideoElement | null>
) => {
  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    "ws://192.168.64.7:8443",
    {
      onOpen: () => {
        sendSessionStartMessage();
      },
    }
  );

  const [sessionId, setSessionId] = useState<string>();
  const rtcRef = useRef<RTCPeerConnection>();

  const [streamingState, setStreamingState] = useState<StreamingState>(
    StreamingState.LOADING
  );

  const sendSessionStartMessage = useCallback(() => {
    sendJsonMessage({ type: "startSession", peerId: camera.peerId });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [sendJsonMessage]);

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

  return { streamingState };
};
