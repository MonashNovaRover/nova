import React, {useCallback, useEffect} from "react";

/**
 * A hook, which plays a given video file url on a given video ref
 * @param videoRef The video ref to play the webcam feed on
 * @param url The url of the video to play
 */
export default function useVideoTexture(videoRef: React.MutableRefObject<HTMLVideoElement | null>, url: string) {

  const play = useCallback(() => {
    videoRef.current?.play()
      .then(() => {
        console.log("played video")
        window.removeEventListener("click", play);
      })
      .catch(e => {console.error(e)});


  }, [videoRef]);

  useEffect(() => {
    if (!videoRef.current) return;

    videoRef.current.playsInline = true;
    videoRef.current.src = url;

    videoRef.current.onended = play;

    window.addEventListener("click", play);


  }, [url, videoRef, play]);

  return videoRef.current;
}