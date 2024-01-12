import React, { LegacyRef } from "react";

interface CameraVideoProps {
  videoRef: LegacyRef<HTMLVideoElement> | undefined;
}

const CameraVideo: React.FC<CameraVideoProps> = ({ videoRef }) => {
  return (
    <video
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
