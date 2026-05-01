import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {RING} from "./CarouselWidget.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useCallback, useEffect} from "react";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {IRosScienceInterfacesCarouselFeedback} from "../../../ros/rosTypes.ts";

/**
 * Hook to subscribe to both carousel feedback topics
 * Returns [innerFeedback, outerFeedback]
 */
export const useCarouselFeedback = (): [IRosScienceInterfacesCarouselFeedback, IRosScienceInterfacesCarouselFeedback] => {
  const innerBifrost = useBifrost({topic: RosTopic.CAROUSEL_INNER_FEEDBACK});
  const outerBifrost = useBifrost({topic: RosTopic.CAROUSEL_OUTER_FEEDBACK});

  useEffect(() => {
    innerBifrost.syncWithTopic();
  }, [innerBifrost]);

  useEffect(() => {
    outerBifrost.syncWithTopic();
  }, [outerBifrost]);

  const innerFeedback = useSelector(
    (state: RootState) => state.carouselInnerFeedback
  );
  const outerFeedback = useSelector(
    (state: RootState) => state.carouselOuterFeedback
  );

  return [innerFeedback, outerFeedback];
}

/**
 * Hook to get set_position service callers for each ring
 * Returns { setInnerPosition, setOuterPosition }
 */
export const useCarouselSetPosition = () => {
  const innerBifrost = useBifrost({service: RosService.CAROUSEL_INNER_SET_POSITION});
  const outerBifrost = useBifrost({service: RosService.CAROUSEL_OUTER_SET_POSITION});

  const setInnerPosition = useCallback((positionDegrees: number) => {
    return innerBifrost.callService({ position: positionDegrees });
  }, [innerBifrost]);

  const setOuterPosition = useCallback((positionDegrees: number) => {
    return outerBifrost.callService({ position: positionDegrees });
  }, [outerBifrost]);

  const setPosition = useCallback((ring: RING, positionDegrees: number) => {
    if (ring === RING.INNER) {
      return setInnerPosition(positionDegrees);
    } else {
      return setOuterPosition(positionDegrees);
    }
  }, [setInnerPosition, setOuterPosition]);

  return { setInnerPosition, setOuterPosition, setPosition };
}

/**
 * Hook to trigger zeroing of the carousel
 * Returns triggerZero function
 */
export const useCarouselZero = () => {
  const innerBifrost = useBifrost({service: RosService.CAROUSEL_INNER_TRIGGER_ZERO});
  const outerBifrost = useBifrost({service: RosService.CAROUSEL_OUTER_TRIGGER_ZERO});

  const triggerZero = useCallback((ring: RING) => {
    const bifrost = ring === RING.INNER ? innerBifrost : outerBifrost;
    return bifrost.callService({});
  }, [innerBifrost, outerBifrost]);

  return { triggerZero };
}

/**
 * Hook for increment zero functionality
 * Returns { incrementZero, resetZero }
 */
export const useCarouselIncrementZero = () => {
  const innerBifrost = useBifrost({service: RosService.CAROUSEL_INNER_INCREMENT_ZERO});
  const outerBifrost = useBifrost({service: RosService.CAROUSEL_OUTER_INCREMENT_ZERO});

  // Curried function: ring => amount => service call
  const incrementZero = useCallback((ring: RING) => (amount: number) => {
    const bifrost = ring === RING.INNER ? innerBifrost : outerBifrost;
    return bifrost.callService({ reset_zero: false, increment_zero: amount });
  }, [innerBifrost, outerBifrost]);

  // Curried function: ring => service call
  const resetZero = useCallback((ring: RING) => () => {
    const bifrost = ring === RING.INNER ? innerBifrost : outerBifrost;
    return bifrost.callService({ reset_zero: true, increment_zero: 0 });
  }, [innerBifrost, outerBifrost]);

  return { incrementZero, resetZero };
}

