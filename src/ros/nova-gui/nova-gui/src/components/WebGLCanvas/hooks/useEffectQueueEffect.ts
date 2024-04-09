import {DependencyList, useEffect} from "react";

/**
 *
 Adds some effect to an effect queue whenever the given dependencies
  */
const useEffectQueueEffect = (effect: () => void, deps: DependencyList, effectQueuePush?: (effect: () => void) => void) => {
  useEffect(() => {
    /*let active = true;

    effectQueuePush(() => {
      if (!active)
        return;
      effect();
    });


    return () => {
      // recall existing effect on the stack
      active = false;
    }*/

    effectQueuePush?.(effect);

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deps]);
}

export default useEffectQueueEffect;