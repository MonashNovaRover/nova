import WebGL360CamCanvas from "./WebGL360CamCanvas.tsx";
import {useRef} from "react";
import useWebcam from "../WebGLCanvas/hooks/useWebcam.tsx";

const Camera360Component = () => {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  useWebcam(videoRef);
  // useVideoTexture(videoRef, TestVideo);

  return (<div className="relative top-0 left-0 right-0 bottom-0">
    <video ref={videoRef} className="absolute top-0 left-0 max-h-full -z-50"/>

    <div className="w-full h-50 min-h-unit-24 min-w-unit-24 resize-y overflow-hidden mb-5">
      <WebGL360CamCanvas
        videoRef={videoRef}
        className="w-full h-full"
      />
    </div>
  </div>);
}

export default Camera360Component;