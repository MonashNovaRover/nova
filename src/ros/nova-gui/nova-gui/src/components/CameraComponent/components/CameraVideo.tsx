import React, { useEffect, useRef } from "react";

interface CameraVideoProps {
  mediaStream: MediaStream | null;
}

const CameraVideo: React.FC<CameraVideoProps> = ({ mediaStream }) => {
  const videoRef = useRef<HTMLVideoElement>(null);

  useEffect(() => {
    if (mediaStream && videoRef.current) {
      // Assign the media stream to the video element
      videoRef.current.srcObject = mediaStream;
    }

    // Cleanup: stop the media stream when the component unmounts
    return () => {
      if (videoRef.current) {
        const currentStream = videoRef.current.srcObject as MediaStream;
        if (currentStream) {
          const tracks = currentStream.getTracks();
          tracks.forEach((track) => track.stop());
        }
      }
    };
  }, [mediaStream]);

  return (
    <video
      controls={false}
      autoPlay
      loop
      muted
      playsInline
      className="z-0 w-full h-full object-cover"
      ref={videoRef}
    />
  );
};

export default CameraVideo;
