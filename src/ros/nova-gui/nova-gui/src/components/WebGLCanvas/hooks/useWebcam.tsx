import {useEffect, useRef, useState} from "react";

export default function useWebcam() {
  // const videoRef = useRef<HTMLVideoElement | undefined>(undefined);
  const videoRef = useRef<HTMLVideoElement>(null);
  const [stream, setStream] = useState<MediaStream | undefined>(undefined) // useRef<MediaStream | undefined>(undefined);

  useEffect(() => {
    //videoRef.current = document.createElement("video");

    if (videoRef.current === null)
      return;

    navigator.mediaDevices.getUserMedia({ video: true })
      .then((newStream) => {
        setStream(newStream);
      });
  }, [videoRef]);

  useEffect(() => {
    if (!videoRef.current || stream === undefined)
      return;

    videoRef.current.srcObject = stream;

    videoRef.current.play().then(() => console.log("Played webcam stream")).catch((e) => {
      console.error("Failed to play webcam stream", e);
    });
  }, [stream, videoRef]);


  return videoRef;
}