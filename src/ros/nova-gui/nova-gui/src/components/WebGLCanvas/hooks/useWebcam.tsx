import {useEffect, useLayoutEffect, useRef, useState} from "react";

export default function useWebcam() {
  const videoRef = useRef<HTMLVideoElement | undefined>(undefined);
  const [stream, setStream] = useState<MediaStream | undefined>(undefined) // useRef<MediaStream | undefined>(undefined);

  useLayoutEffect(() => {
    videoRef.current = document.createElement("video");

    if (videoRef.current === null)
      return;

    navigator.mediaDevices.getUserMedia({ video: true })
      .then((newStream) => {
        setStream(newStream);
      });
  }, []);

  useEffect(() => {
    if (!videoRef.current || stream === undefined)
      return;

    videoRef.current.srcObject = stream;

    videoRef.current.play().catch((e) => {
      console.error("Failed to play webcam stream", e);
    });
  }, [stream, videoRef]);


  return videoRef.current;
}