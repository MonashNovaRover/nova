import React, { useEffect, useRef, useState } from "react";
import { useYoloContext } from "./YoloProvider";
import YoloOverlayCanvas from "./YoloOverlayCanvas";
import {BaseCameraComponentProps, CameraComponent} from "../../cameras/CameraComponent/CameraComponent.tsx";

export default function YoloCameraComponent(
  props: BaseCameraComponentProps
) {
  const videoRef =
    useRef<HTMLVideoElement>(null);

  const {
    registerVideoRef,
    detections,
  } = useYoloContext();

  const [cameraIndex, setCameraIndex] =
    useState<number | null>(null);

  useEffect(() => {
    const index =
      registerVideoRef(videoRef);

    setCameraIndex(index);
  }, []);

  const cameraDetections =
    cameraIndex !== null
      ? detections?.[cameraIndex] ??
      []
      : [];

  return (
    <CameraComponent
      {...props}
      cameraVideoComponent={({
                               filters,
                             }) => (
        <>
          <video
            ref={videoRef}
            // className="absolute inset-0 w-full h-full object-cover"
            autoPlay
            muted
            playsInline
          />

          <YoloOverlayCanvas
            detections={
              cameraDetections
            }
            videoRef={videoRef}
          />
        </>
      )}
    />
  );
}