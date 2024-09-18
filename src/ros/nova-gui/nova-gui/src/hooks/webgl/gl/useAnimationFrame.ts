import {useCallback, useEffect, useRef} from "react";

/**
 * Runs a loop with requestAnimationFrame on the callback provided
 * @param callback A callback which can take the time and deltaTime in milliseconds as arguments
 */
export default function useAnimationFrame(
  callback: (milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number) => void): void {

  // Use useRef for mutable variables that we want to persist
  // without triggering a re-render on their change
  const requestRef = useRef(-1);
  const previousTimeRef = useRef(0);

  const animationCallback = useCallback((time: DOMHighResTimeStamp) => {
    const deltaTime = time - (previousTimeRef.current ?? time);
    callback(time, deltaTime);

    previousTimeRef.current = time;
    requestRef.current = requestAnimationFrame(animationCallback);
  }, [callback])

  useEffect(() => {
    requestRef.current = requestAnimationFrame(animationCallback);
    return () => cancelAnimationFrame(requestRef.current);
  }, [animationCallback]); // Make sure the effect runs only once
}

