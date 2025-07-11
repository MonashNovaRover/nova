import { useCallback, useEffect, useRef, useState } from "react";
import { PeerMessage, ServerMessage } from "./serverMessages.ts";
import useWebSocket from "react-use-websocket";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
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

/**
 * Custom hook for managing camera streaming.
 *
 * @param cameraSerial - The serial number of the camera.
 * @param videoRef - A mutable ref object for the HTML video element.
 * @param autoStart - Optional boolean flag indicating whether to automatically start the camera stream. Default is `false`.
 */
export const useCameraStream = (
  cameraSerial: string,
  videoRef: React.MutableRefObject<HTMLVideoElement | null>,
  autoStart?: boolean
) => {
  const [isWsOpen, setWsOpen] = useState(false);
  const roverIP = useSelector((state: RootState) => state.uiState.roverIP);

  const { sendJsonMessage, lastJsonMessage } = useWebSocket<ServerMessage>(
    `ws://${roverIP}:8443`,
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
  const [erroredOut, setErroredOut] = useState(false);
  const rtcRef = useRef<RTCPeerConnection>();
  const peerId = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras[cameraSerial]
  );

  const sendSessionStartMessage = useCallback(() => {
    if (!isWsOpen) return;
    if (!peerId) {
      toast.error(`${cameraSerial} unable to start up`);
      return;
    }
    setStreamingState(StreamingState.LOADING);
    sendJsonMessage({ type: "startSession", peerId });
  }, [sendJsonMessage, peerId, isWsOpen, cameraSerial]);

  const [streamingState, setStreamingState] = useState<StreamingState>(
    StreamingState.STOPPED
  );

  const closeSession = useCallback(() => {
    if (!isWsOpen) return;
    if (!peerId) {
      toast.error(`${cameraSerial} unable to start up`);
      return;
    }
    setStreamingState(StreamingState.STOPPED);

    if (rtcRef.current) {
      rtcRef.current.close();
    }

    if (videoRef.current) videoRef.current.srcObject = null;

    sendJsonMessage({ type: "endSession", sessionId });
  }, [cameraSerial, isWsOpen, peerId, sendJsonMessage, sessionId, videoRef]);

  useEffect(() => {
    if (autoStart && isWsOpen && isCameraOnline) {
      sendSessionStartMessage();
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isWsOpen, isCameraOnline]);

  useEffect(() => {
    if (streamingState === StreamingState.STOPPED && isCameraOnline) {
      if (autoStart) {
        sendSessionStartMessage();
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [autoStart]);

  useEffect(() => {
    if (streamingState === StreamingState.STREAMING) {
      if (!autoStart) {
        closeSession();
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [autoStart]);

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
        throw new Error(`Unknown peer message: ${message.type}`);
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
        if (lastJsonMessage.sessionId !== sessionId) {
          setSessionId(lastJsonMessage.sessionId); // This kinda has to be done everytime to ensure that this points to the latest
        }
        const rtcPeerConnection = handOverRTCPeerConnection();
        handlePeerMessage(rtcPeerConnection, lastJsonMessage);
        break;
      }
      default: {
        if (!erroredOut) {
          toast.error("Cameras2 Errored Out. Please check Cameras2");
          setErroredOut(true);
        }
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastJsonMessage]);

  return {
    streamingState,
    sendSessionStartMessage,
    isCameraOnline,
    closeSession,
  };
};
