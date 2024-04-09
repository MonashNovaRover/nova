import {useEffect, useRef} from "react";

export default function useAnimationFrame(callback: (time: number, deltaTime: number) => void): void {
  // Use useRef for mutable variables that we want to persist
  // without triggering a re-render on their change
  const requestRef = useRef(-1);
  const previousTimeRef = useRef(0);

  const animationCallback = (time: number) => {
    const deltaTime = time - (previousTimeRef.current ?? time);
    callback(time * 0.001, deltaTime * 0.001);

    previousTimeRef.current = time;
    requestRef.current = requestAnimationFrame(animationCallback);
  }

  useEffect(() => {
    requestRef.current = requestAnimationFrame(animationCallback);
    return () => cancelAnimationFrame(requestRef.current);
  }, []); // Make sure the effect runs only once
}

