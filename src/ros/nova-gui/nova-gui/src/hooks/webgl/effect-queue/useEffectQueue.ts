import {useRef} from "react";
import EffectQueue from "./EffectQueue.ts";


/**
 * A queue to help run synchronous set up functions, with setup being set by independent effects.
 */
const useEffectQueue = <T extends unknown[]>() : EffectQueue<T> => {
  const queueRef = useRef<EffectQueue<T> | undefined>(undefined);
  if (queueRef.current === undefined)
    queueRef.current = new EffectQueue<T>();

  // Since we never reassign the queueRef, we can return the .current value directly, as it never changes.
  // https://dev.to/thoughtspile/can-we-useref-but-without-the-current-lets-try-4mam
  return queueRef.current!;
}

export default useEffectQueue;