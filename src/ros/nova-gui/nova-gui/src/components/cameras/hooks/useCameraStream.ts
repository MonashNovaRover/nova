import { useCallback, useEffect, useRef, useState } from "react";
import { PeerMessage, ServerMessage } from "./serverMessages.ts";
import useWebSocket from "react-use-websocket";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
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

const HEARTBEAT_INTERVAL = 1000; // ms
const MAX_LATENCY = 120; // ms

// H.265 hardware decoders tend to be intolerant of back-to-back forced
// IDR/keyframe requests (e.g. during rapid zoom, which spikes display
// latency several times in a row). These guard against request storms.
const MIN_KEYFRAME_REQUEST_INTERVAL = 500; // ms, min gap between keyframe requests
const RESET_SESSION_COOLDOWN = 3000; // ms, suppress heartbeat action right after a reset

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
  const rtcRef = useRef<RTCPeerConnection>(undefined);
  const keyframeRequestInFlight = useRef(false);
  const lastKeyframeRequestTime = useRef(0);
  const lastResetSessionTime = useRef(0);
  const lastFrameCallbackTime = useRef(0);
  const suppressNextAutoKeyframe = useRef(false);
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

  const requestRandomAccessKeyframe = useCallback(() => {
    if (!rtcRef.current) return;

    // Guard against request storms: H.265 HW decoders can wedge if a new
    // keyframe is requested before the previous one has been processed.
    if (keyframeRequestInFlight.current) return;
    const now = Date.now();
    if (now - lastKeyframeRequestTime.current < MIN_KEYFRAME_REQUEST_INTERVAL) {
      return;
    }

    const receivers = rtcRef.current.getReceivers();
    const videoReceiver = receivers.find(r => r.track?.kind === 'video');

    if (videoReceiver && 'requestKeyFrame' in videoReceiver) {
      keyframeRequestInFlight.current = true;
      lastKeyframeRequestTime.current = now;
      (videoReceiver as any).requestKeyFrame()
        .then(() => console.log(`Random access keyframe requested for ${cameraSerial}`))
        .catch((err: any) => console.error("Keyframe request failed: ", err))
        .finally(() => {
          keyframeRequestInFlight.current = false;
        });
    }
  }, [cameraSerial]);

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
    rtcRef.current = undefined; // Reset the RTC connection

    if (videoRef.current) videoRef.current.srcObject = null;

    sendJsonMessage({ type: "endSession", sessionId });
  }, [cameraSerial, isWsOpen, peerId, sendJsonMessage, sessionId, videoRef]);

  const destroyRTCPeerConnection = useCallback(() => {
    if (rtcRef.current) {
      rtcRef.current.onicecandidate = null;
      rtcRef.current.ontrack = null;
      rtcRef.current.close();
      rtcRef.current = undefined;
    }

    if (videoRef.current) {
      videoRef.current.srcObject = null;
    }
  }, [videoRef]);

  const resetSession = useCallback(async () => {
    if (!isWsOpen || !peerId) return;

    setStreamingState(StreamingState.LOADING);
    lastResetSessionTime.current = Date.now();
    lastFrameCallbackTime.current = 0;
    // The new connection's first frame is already an IDR by nature of
    // negotiation, so skip the redundant auto keyframe request in ontrack
    // to avoid immediately re-triggering the same latency spike.
    suppressNextAutoKeyframe.current = true;

    const oldSessionId = sessionId;

    // Kill the current WebRTC connection immediately.
    destroyRTCPeerConnection();

    // Tell the server to terminate the old session.
    if (oldSessionId) {
      sendJsonMessage({
        type: "endSession",
        sessionId: oldSessionId,
      });
    }

    // Give the server a chance to tear down the old session.
    await new Promise(resolve => setTimeout(resolve, 100));

    // Start a completely new session.
    sendJsonMessage({
      type: "startSession",
      peerId,
    });
  }, [
    isWsOpen,
    peerId,
    sessionId,
    sendJsonMessage,
    destroyRTCPeerConnection,
  ]);

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
        await rtcPeerConnection.setRemoteDescription(message.sdp);
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
        iceCandidatePoolSize: 0, // Drop old packets for newest
      });

      rtcConnection.onicecandidate = iceCandidateCallback;
      rtcConnection.ontrack = (event) => {

        const receiver = event.receiver;
        if (receiver.track.kind === 'video') {
          // Do not delay video stream
          if ('playoutDelayHint' in receiver) {
            receiver.playoutDelayHint = 0;
          }


          try {
            // Tag content as motion to bias maintaining framerate
            if ('contentHint' in event.track) event.track.contentHint = 'motion';

            const senders = rtcConnection.getSenders();
            const videoSender = senders.find(sender => sender.track?.kind === 'video');
            if (videoSender) {
              const parameters = videoSender.getParameters();
              parameters.degradationPreference = 'balanced';
              videoSender.setParameters(parameters);
            }
          } catch (e) {
            console.warn("Failed to set leaky bucket parameters: ", e);
          }
        }

        // Set mode to streaming
        setStreamingState(StreamingState.STREAMING);
        if (suppressNextAutoKeyframe.current) {
          suppressNextAutoKeyframe.current = false;
        } else {
          requestRandomAccessKeyframe();
        }
        if (videoRef.current) videoRef.current.srcObject = event.streams[0];
      };

      rtcRef.current = rtcConnection;
      return rtcRef.current;
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [videoRef, requestRandomAccessKeyframe]);

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
        if (!lastJsonMessage.sessionId) break;

        if (
          sessionId &&
          lastJsonMessage.sessionId !== sessionId
        ) {
          console.warn(
            "Ignoring peer message for stale session",
            lastJsonMessage.sessionId
          );
          break;
        }

        const rtcPeerConnection = handOverRTCPeerConnection();

        handlePeerMessage(
          rtcPeerConnection,
          lastJsonMessage
        );

        break;
      }
      default: {
        if (!erroredOut) {
          toast.error("Cameras Errored Out. Please check Cameras");
          setErroredOut(true);
        }
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [lastJsonMessage]);

  // Magic sauce that resets desynced cameras
  useEffect(() => {
    let cancelled = false;
    let rvfcHandle: number | undefined;
    let attachedVideo: HTMLVideoElement | undefined;

    const scheduleFrameCallback = (video: HTMLVideoElement) => {
      attachedVideo = video;
      rvfcHandle = video.requestVideoFrameCallback((now, metadata) => {
        if (cancelled) return;

        lastFrameCallbackTime.current = Date.now();

        // Skip while a reset is still settling in, so we don't stack
        // another reset/keyframe request on top of a connection that
        // hasn't finished renegotiating yet.
        if (Date.now() - lastResetSessionTime.current >= RESET_SESSION_COOLDOWN) {
          const display_delay = now - (metadata.receiveTime ?? now);
          if (display_delay > MAX_LATENCY) {
            if (display_delay > MAX_LATENCY * 5) {
              // Session terminated
              resetSession();
            } else {
              // Minor lag
              requestRandomAccessKeyframe();
            }
          }
        }

        // Keep chaining so we get called on every frame, not just once.
        scheduleFrameCallback(video);
      });
    };

    const interval = setInterval(() => {
      const video = videoRef.current;
      if (!video) return;

      // Self-heal: if the <video> element wasn't attached yet when this
      // effect first ran, or was swapped for a different element (e.g. a
      // "hide and show" that remounts it), (re)attach the frame-callback
      // chain to whatever element is current.
      if (video !== attachedVideo) {
        if (attachedVideo && rvfcHandle !== undefined) {
          attachedVideo.cancelVideoFrameCallback(rvfcHandle);
        }
        lastFrameCallbackTime.current = 0;
        scheduleFrameCallback(video);
      }

      // Watchdog: requestVideoFrameCallback only fires when a new frame is
      // actually presented. If the stream has genuinely stalled (frozen
      // decoder, dead track, etc.) it can stop firing entirely, which means
      // the delay-based check above never runs and never recovers. Detect
      // that "no frames at all" case on a plain wall clock instead.
      if (
        lastFrameCallbackTime.current !== 0 &&
        Date.now() - lastFrameCallbackTime.current > MAX_LATENCY * 5 &&
        Date.now() - lastResetSessionTime.current >= RESET_SESSION_COOLDOWN
      ) {
        resetSession();
      }
    }, HEARTBEAT_INTERVAL);

    return () => {
      cancelled = true;
      clearInterval(interval);
      if (rvfcHandle !== undefined && attachedVideo) {
        attachedVideo.cancelVideoFrameCallback(rvfcHandle);
      }
    };
  }, [resetSession, requestRandomAccessKeyframe, videoRef]);

  return {
    streamingState,
    sendSessionStartMessage,
    isCameraOnline,
    closeSession,
    resetSession,
    requestRandomAccessKeyframe,
  };
};
