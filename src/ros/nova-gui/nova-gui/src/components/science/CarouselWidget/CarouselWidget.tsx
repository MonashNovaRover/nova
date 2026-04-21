import React, {useState} from "react";
import {Card, CardBody, CardHeader, CardProps, Spinner} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import CarouselInputs from "./CarouselInputs.tsx";
import CarouselControls from "./CarouselControls.tsx";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {
  useCarouselFeedback,
  useCarouselSetPosition,
  useCarouselZero,
  useCarouselIncrementZero
} from "./useCarouselBifrost.ts";

export enum RING {
  INNER = 0,
  OUTER = 1,
}
export const RING_NAMES = ["inner", "outer"];
export const NUM_CUVETTES = [15, 24]
const DEGREES_PER_CUVETTE = [360 / 15, 360 / 24] // [24, 15] degrees per cuvette

export type CuvettePositions = [number, number]

export interface CarouselWidgetProps extends CardProps{
}

/**
 * Convert cuvette index (0-indexed) to degrees (0-360)
 */
const cuvetteToDegrees = (ring: RING, cuvetteIndex: number): number => {
  return (cuvetteIndex * DEGREES_PER_CUVETTE[ring]) % 360;
}

/**
 * Convert degrees (0-360) to cuvette index (0-indexed)
 */
const degreesToCuvette = (ring: RING, degrees: number): number => {
  const normalizedDegrees = ((degrees % 360) + 360) % 360;
  return Math.round(normalizedDegrees / DEGREES_PER_CUVETTE[ring]) % NUM_CUVETTES[ring];
}

/**
 * Widget to control the carousel.
 * @param props
 * @constructor
 */
const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  // Get feedback from ROS topics
  const [innerFeedback, outerFeedback] = useCarouselFeedback();
  const { setPosition } = useCarouselSetPosition();
  const { triggerZero } = useCarouselZero();
  const { incrementZero, resetZero } = useCarouselIncrementZero();

  // Derive current cuvette positions from feedback positions (in degrees)
  const currentCuvettes: CuvettePositions = [
    degreesToCuvette(RING.INNER, innerFeedback.position),
    degreesToCuvette(RING.OUTER, outerFeedback.position)
  ];

  // Check if either ring is zeroing
  const isZeroing = innerFeedback.zeroing || outerFeedback.zeroing;

  const [selectedTab, setSelectedTab] = useState(0)
  const showCalibration = selectedTab === 1

  const setCuvettePosition = (ring: RING, cuvetteIndex: number) => {
    const degrees = cuvetteToDegrees(ring, cuvetteIndex);
    setPosition(ring, degrees);
  }

  const onCuvetteClick = (ring: RING) => (index: number) => {
    setCuvettePosition(ring, index);
  }

  const moveXCuvettes = (ring: RING)=> (x: number) => {
    const newCuvette = (currentCuvettes[ring] + x + NUM_CUVETTES[ring]) % NUM_CUVETTES[ring];
    setCuvettePosition(ring, newCuvette);
  }

  const moveXDegrees = (ring: RING)=> (x: number)=> {
    const feedback = ring === RING.INNER ? innerFeedback : outerFeedback;
    const newDegrees = (feedback.position + x + 360) % 360;
    setPosition(ring, newDegrees);
  }

  return <Card {...props}>
    <CardHeader className="flex flex-row items-center gap-2">
      Carousel
      {isZeroing && <Spinner size="sm" color="warning" />}
      {isZeroing && <span className="text-warning text-sm">Zeroing...</span>}
    </CardHeader>
    <CardBody className="grid grid-cols-5 gap-3">
      <div className="flex flex-col col-span-2 gap-3">

        <SegmentedPicker
          selectedIndex={selectedTab}
          onIndexChange={setSelectedTab}
          children={[
            "Analysis", "Calibration"
          ]}
          color="primary"
          className="pb-0"
          fullWidth
          variant="bordered"
        />

        <CarouselInputs
          currentCuvettes={currentCuvettes}
          showCalibration={showCalibration}
          setCurrentCuvette={(positions) => {
            setCuvettePosition(RING.INNER, positions[RING.INNER]);
            setCuvettePosition(RING.OUTER, positions[RING.OUTER]);
          }}
          moveXCuvettes={moveXCuvettes}
          triggerZero={triggerZero}
          incrementZero={incrementZero}
          resetZero={resetZero}
          isZeroing={isZeroing}
        />
      </div>

      <div className="col-span-3 flex flex-col gap-3">
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.OUTER)}
          moveXSteps={moveXDegrees(RING.OUTER)}
          showCalibration={showCalibration}
          variant={RING.OUTER}
          reverse
          disabled={isZeroing}
        />
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.INNER)}
          moveXSteps={moveXDegrees(RING.INNER)}
          showCalibration={showCalibration}
          variant={RING.INNER}
          reverse
          disabled={isZeroing}
        />
        <CarouselDial
          inner={{current: currentCuvettes[RING.INNER], onClick: onCuvetteClick(RING.INNER)}}
          outer={{current: currentCuvettes[RING.OUTER], onClick: onCuvetteClick(RING.OUTER)}}
        />
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2
