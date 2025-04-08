import React, { LegacyRef } from "react";
import { CameraFilters } from "../CameraComponent";

export interface CameraVideoProps {
  videoRef: LegacyRef<HTMLVideoElement> | undefined;
  filters: CameraFilters;
}

const CameraVideo: React.FC<CameraVideoProps> = ({ videoRef, filters }) => {
  const scaling = filters.flipCamera ? "scaleX(-1)" : "scaleX(1)";
  const rotation = `rotate(${filters.rotation}deg)`;
  const contrast = `contrast(${filters.contrast}%)`;
  const brightness = `brightness(${filters.brightness}%)`;
  const inversion = `invert(${filters.invertCamera ? 1 : 0})`;

  return (
    <video
      style={{
        transform: `${scaling} ${rotation}`,
        filter: `${contrast} ${brightness} ${inversion}`,
      }}
      controls={false}
      autoPlay
      loop
      muted
      playsInline
      ref={videoRef}
      className="z-0 w-full h-full object-cover"
    />
  );
};

export default CameraVideo;
