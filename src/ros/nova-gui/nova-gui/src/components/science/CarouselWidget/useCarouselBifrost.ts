import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {CuvettePositions, RING_NAMES} from "./CarouselWidget.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useEffect} from "react";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";

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
