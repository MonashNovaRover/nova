import {DependencyList, useEffect} from "react";
import EffectQueue from "./EffectQueue.ts";

/**
 *
 Adds some effect to an effect queue whenever the given dependencies
  */
const useEffectQueueEffect = <T extends unknown[]>(effect: (...args: T) => void, deps: DependencyList, queue: EffectQueue<T>) => {
  useEffect(() => {
    const frameID = queue.push(effect);

    return () => {
      queue.cancel(frameID);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deps]);
}

export default useEffectQueueEffect;