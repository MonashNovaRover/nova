import React, { LegacyRef } from "react";
import { CameraFilters } from "../CameraComponent";

interface CameraVideoProps {
  videoRef: LegacyRef<HTMLVideoElement> | undefined;
  filters: CameraFilters;
}

const CameraVideo: React.FC<CameraVideoProps> = ({ videoRef, filters }) => {
  const scaling = filters.flipCamera ? "scaleX(-1)" : "scaleX(1)";
  const rotation = `rotate(${filters.rotation}deg)`;
  return (
    <video
      style={{
        transform: `${scaling} ${rotation}`,
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
