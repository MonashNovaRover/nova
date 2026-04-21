import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {CuvettePositions, RING, RING_NAMES} from "./CarouselWidget.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useCallback, useEffect} from "react";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {IRosScienceInterfacesCarouselFeedback} from "../../../ros/rosTypes.ts";

export const useCarouselPosition = (): [CuvettePositions, CuvettePositions] => {
  // Get the current cuvettes
  const bifrost = useBifrost({topic: RosTopic.CAROUSEL});
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const carouselStore = useSelector(
    (state: RootState) => state.carouselStore
  );
  console.log(carouselStore)

  const cuvettePositionIndices = RING_NAMES.map(v => carouselStore.names.indexOf(v + "_cuvette"))
  if (cuvettePositionIndices.includes(-1)) {
    console.error("Could not find all ring names cuvettes", carouselStore.names)
    return [[0,0], [0,0]]
  }

  const degreePositionIndices = RING_NAMES.map(v => carouselStore.names.indexOf(v + "_degree"))
  if (degreePositionIndices.includes(-1)) {
    console.error("Could not find all ring names degree", carouselStore.names)
    return [[0,0], [0,0]]
  }

  if (cuvettePositionIndices.length != RING_NAMES.length && degreePositionIndices.length != RING_NAMES.length) {
    console.error(`There are not ${RING_NAMES.length} of each degree and cuvette`, carouselStore.names)
    return [[0,0], [0,0]]
  }

  return [
    cuvettePositionIndices.map(i => carouselStore.positions[i]) as CuvettePositions,
    degreePositionIndices.map(i => carouselStore.positions[i]) as CuvettePositions
  ]
}

export const useCarouselServices = () => {
  const bifrost = useBifrost({service: RosService.CAROUSEL});

  return bifrost.callService
}

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
  const bifrost = useBifrost({service: RosService.CAROUSEL_TRIGGER_ZERO});

  const triggerZero = useCallback(() => {
    return bifrost.callService({});
  }, [bifrost]);

  return { triggerZero };
}

/**
 * Hook for increment zero functionality
 * Returns { incrementZero, resetZero }
 */
export const useCarouselIncrementZero = () => {
  const bifrost = useBifrost({service: RosService.CAROUSEL_INCREMENT_ZERO});

  const incrementZero = useCallback((amount: number) => {
    return bifrost.callService({ reset_zero: false, increment_zero: amount });
  }, [bifrost]);

  const resetZero = useCallback(() => {
    return bifrost.callService({ reset_zero: true, increment_zero: 0 });
  }, [bifrost]);

  return { incrementZero, resetZero };
}
