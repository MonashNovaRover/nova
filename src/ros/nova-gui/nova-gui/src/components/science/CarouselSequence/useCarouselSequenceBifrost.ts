import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { useCallback, useEffect } from "react";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { IRosScienceInterfacesCarouselSequenceStatus } from "../../../ros/rosTypes.ts";

/**
 * Hook to subscribe to carousel sequence status topic
 * Returns the current sequence status
 */
export const useCarouselSequenceStatus = (): IRosScienceInterfacesCarouselSequenceStatus => {
  const bifrost = useBifrost({ topic: RosTopic.CAROUSEL_SEQUENCE_STATUS });

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const status = useSelector(
    (state: RootState) => state.carouselSequenceStatusStore
  );

  return status;
};

/**
 * Hook to get sequence control functions
 * Returns { startSequence, stopSequence }
 */
export const useCarouselSequenceControl = () => {
  const startBifrost = useBifrost({ service: RosService.CAROUSEL_SEQUENCE_START });
  const stopBifrost = useBifrost({ service: RosService.CAROUSEL_SEQUENCE_STOP });

  const startSequence = useCallback(
    (ring: "inner" | "outer", iterations: number, pumpDuration: number) => {
      return startBifrost.callService({
        ring,
        iterations,
        pump_duration: pumpDuration,
      });
    },
    [startBifrost]
  );

  const stopSequence = useCallback(() => {
    return stopBifrost.callService({});
  }, [stopBifrost]);

  return { startSequence, stopSequence };
};
