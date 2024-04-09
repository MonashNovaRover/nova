import {useCallback, useMemo, useRef} from "react";

export interface EffectQueue {
  push: (effect: () => void) => void,
  clear: () => boolean,
}

/**
 * A queue to help run synchronous set up functions, with setup being set by independent effects.
 */
const useEffectQueue = () => {
  const queueRef = useRef<(() => void)[]>([]);



  // const frameIDRef = useRef<number | undefined>(undefined);

  // Calls all functions currently in the queue.
  // Returns true if the queue is already clear, and false if the queue was cleared.
  const clear = useCallback(() => {
    if (queueRef.current.length === 0)
      return true;

    for (let i = 0; i < queueRef.current.length; i++)
      queueRef.current[i]();

    queueRef.current = [];
    return false;
  }, [])

  // Adds a function to the queue
  const push = useCallback((item: () => void) => {
    queueRef.current.push(item);


  }, [])



  // useEffect(() => {

  //   console.log("howdy");
  // }, []);
  // This is the call that actually does stuff
  /* useEffect(() => {
    if (clear())
      return;

    // Do stuff for when stuff in the queue is called!
  }, [rawQueue, clear]) */

  // This is the object returned by the hook
  return useMemo(() => ({
    push: push,
    clear: clear,
  } as EffectQueue), [push, clear])
}

export default useEffectQueue;