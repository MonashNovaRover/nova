import {useCallback, useEffect, useState} from "react";
import {StreamingState} from "../../CameraComponent/hooks/useCameraStream.ts";
import toast from "react-hot-toast";

/**
 * A hook, which plays the webcam on a given video ref
 * @param videoRef The video ref to play the webcam feed on
 */
export default function useWebcam(videoRef: React.MutableRefObject<HTMLVideoElement | null>) {
  // const videoRef = useRef<HTMLVideoElement | undefined>(undefined);
  // const videoRef = useRef<HTMLVideoElement>(null);
  const [stream, setStream] = useState<MediaStream | undefined>(undefined)
  const [streamingState, setStreamingState] = useState<StreamingState>(
    StreamingState.STOPPED
  );

  const [isCameraOnline, setIsCameraOnline] = useState<boolean>(true);

  useEffect(() => {
    if (videoRef.current === null)
      return;

    navigator.mediaDevices.getUserMedia({ video: true })
      .then((newStream) => {
        setStream(newStream);
      }).catch((e) => {
        console.error(e);
        toast.error(`webcam unable to start up: ${e}`);
      setIsCameraOnline(false);
      });
  }, [videoRef]);

  const sendSessionStartMessage = useCallback(() => {
    if (videoRef.current === null)
      return;

    setStreamingState(StreamingState.LOADING);
    navigator.mediaDevices.getUserMedia({ video: true })
      .then((newStream) => {
        setStream(newStream);
      });
  }, [videoRef.current]);

  useEffect(() => {
    sendSessionStartMessage();
  }, [sendSessionStartMessage]);

  useEffect(() => {
    if (!videoRef.current || stream === undefined)
      return;

    videoRef.current.srcObject = stream;

    setStreamingState(StreamingState.LOADING);
    videoRef.current.play().catch((e) => {
      console.error("Failed to play webcam stream", e);
      setStreamingState(StreamingState.STREAMING);
    });
  }, [stream, videoRef]);


  // const { streamingState, sendSessionStartMessage, isCameraOnline } = useCameraStream(cameraSerial, videoRef);
  return {
    streamingState,
    sendSessionStartMessage,
    isCameraOnline
  };
}