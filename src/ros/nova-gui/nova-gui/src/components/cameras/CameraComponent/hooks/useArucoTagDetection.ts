import { useEffect, RefObject } from "react";
import toast from "react-hot-toast";
import AR from "js-aruco2";

export function useArucoTagDetection(videoRef: RefObject<HTMLVideoElement | null>) {
  useEffect(() => {
    AR.AR.DICTIONARIES.ARCh = {
      nBits: 16,
      tau: 2,
      codeList: [[181, 50], [15, 154], [51, 45], [153, 70], [84, 158], [121, 205]]
    };
    const detector = new AR.AR.Detector({ dictionaryName: 'ARCh' });

    const interval = setInterval(() => {
      if (!videoRef.current) return;
      const video = videoRef.current;
      const canvas = document.createElement("canvas");
      canvas.width = video.videoWidth;
      canvas.height = video.videoHeight;
      const context = canvas.getContext("2d");
      if (context && canvas.width && canvas.height) {
        context.drawImage(video, 0, 0);
        const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
        const markers = detector.detect(imageData);
        const ids = markers.map((x) => x.id);
        if (ids.length !== 0) toast(ids.join(", "));
      }
    }, 1000);

    return () => clearInterval(interval);
  }, [videoRef]);
}