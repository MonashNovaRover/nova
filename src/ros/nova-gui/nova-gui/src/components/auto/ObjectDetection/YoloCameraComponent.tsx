import { useEffect, useRef, useState } from "react";
import type { RefObject } from "react";
import { useYoloContext } from "./YoloProvider";
import YoloOverlayCanvas from "./YoloOverlayCanvas";
import { BaseCameraComponentProps, CameraComponent } from "../../cameras/CameraComponent/CameraComponent.tsx";
import CameraVideo, { CameraVideoProps } from "../../cameras/CameraComponent/components/CameraVideo.tsx";

// CameraVideoProps exposes a LegacyRef which can be a callback or undefined.
// We only support object refs here because YOLO needs stable .current access.
function isVideoRef(ref: CameraVideoProps["videoRef"]): ref is RefObject<HTMLVideoElement | null> {
  return !!ref && typeof ref === "object" && "current" in ref;
}

function YoloVideoLayer({ videoRef, filters }: CameraVideoProps) {
  const { registerVideoRef, detections, inputSize } = useYoloContext();
  const [cameraIndex, setCameraIndex] = useState<number | null>(null);
  const registeredRef = useRef(false);

  useEffect(() => {
    // Ensure each video ref is registered only once.
    if (registeredRef.current) {
      return;
    }
    if (!isVideoRef(videoRef)) {
      return;
    }
    const index = registerVideoRef(videoRef);
    registeredRef.current = true;
    setCameraIndex(index);
  }, [registerVideoRef, videoRef]);

  const cameraDetections = cameraIndex !== null ? detections?.[cameraIndex] ?? [] : [];

  return (
    <>
      {/* Base video layer */}
      <CameraVideo videoRef={videoRef} filters={filters} />
      {/* Detection overlay aligned to the video */}
      {isVideoRef(videoRef) && (
        <YoloOverlayCanvas
          detections={cameraDetections}
          videoRef={videoRef}
          modelInputSize={inputSize}
        />
      )}
    </>
  );
}

export default function YoloCameraComponent(props: BaseCameraComponentProps) {
  return (
    <CameraComponent
      {...props}
      // Inject the YOLO-aware camera layer for this component.
      cameraVideoComponent={(cameraProps) => <YoloVideoLayer {...cameraProps} />}
    />
  );
}
