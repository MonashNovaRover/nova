import React, { useEffect, useRef, useState } from "react";
import { useYoloContext } from "./YoloProvider";
import YoloOverlayCanvas from "./YoloOverlayCanvas";
import {BaseCameraComponentProps, CameraComponent} from "../../cameras/CameraComponent/CameraComponent.tsx";
import CameraVideo, {CameraVideoProps} from "../../cameras/CameraComponent/components/CameraVideo.tsx";

function YoloVideoLayer({videoRef, filters}: CameraVideoProps) {
  const {registerVideoRef, detections} = useYoloContext();
  const [cameraIndex, setCameraIndex] = useState<number | null>(null);
  const registeredRef = useRef(false);

  useEffect(() => {
    if (registeredRef.current) {
      return;
    }
    const index = registerVideoRef(videoRef);
    registeredRef.current = true;
    setCameraIndex(index);
  }, [registerVideoRef, videoRef]);

  const cameraDetections =
    cameraIndex !== null
      ? detections?.[cameraIndex] ?? []
      : [];

  return (
    <>
      <CameraVideo videoRef={videoRef} filters={filters} />
      <YoloOverlayCanvas
        detections={cameraDetections}
        videoRef={videoRef}
      />
    </>
  );
}

export default function YoloCameraComponent(props: BaseCameraComponentProps) {
  return (
    <CameraComponent
      {...props}
      cameraVideoComponent={(cameraProps) => (
        <YoloVideoLayer {...cameraProps} />
      )}
    />
  );
}
