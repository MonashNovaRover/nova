import React, {useMemo, useState} from "react";
import {Button, Card, CardBody, CardHeader, CardProps, Dropdown, DropdownItem, DropdownMenu, DropdownTrigger
} from "@nextui-org/react";
import {Check, MoreHorizontal} from "react-feather";
import CarouselDial from "./CarouselDial.tsx";
import CarouselControls from "./CarouselControls.tsx";
import {
  useCarouselFeedback,
  useCarouselSetPosition,
  useCarouselIncrementZero,
  useCarouselZero
} from "./useCarouselBifrost.ts";
import {CarouselHallEffects} from "./CarouselHallEffects.tsx";

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
  const { incrementZero } = useCarouselIncrementZero();

  // Manual position override state
  const [useManualPosition, setUseManualPosition] = useState(true);
  const [manualPositions, setManualPositions] = useState<CuvettePositions>([0, 0]); // Current Degrees
  const [ignoreZeroingStatus, setIgnoreZeroingStatus] = useState(true);

  // Derive current cuvette positions from feedback positions (in degrees)
  const currentCuvettes: CuvettePositions = useMemo(() => [
    degreesToCuvette(RING.INNER, useManualPosition ? manualPositions[RING.INNER] : innerFeedback.position),
    degreesToCuvette(RING.OUTER, useManualPosition ? manualPositions[RING.OUTER] : outerFeedback.position)
  ], [useManualPosition, manualPositions, innerFeedback, outerFeedback]);

  // Check if either ring is zeroing
  const isZeroing = ignoreZeroingStatus ? false : (innerFeedback.zeroing || outerFeedback.zeroing);

  // const [selectedTab, setSelectedTab] = useState(0)
  // const showCalibration = selectedTab === 1

  const setCuvettePosition = (ring: RING, cuvetteIndex: number) => {
    const degrees = cuvetteToDegrees(ring, cuvetteIndex);
    setPosition(ring, degrees);
    setManualPositions([
      ring == RING.INNER ? degrees : manualPositions[RING.INNER],
      ring == RING.OUTER ? degrees : manualPositions[RING.OUTER],
    ])
  }

  const onCuvetteClick = (ring: RING) => (index: number) => {
    setCuvettePosition(ring, index);
  }

  const moveXCuvettes = (ring: RING)=> (x: number) => {
    const newCuvette = (currentCuvettes[ring] + x + NUM_CUVETTES[ring]) % NUM_CUVETTES[ring];
    setCuvettePosition(ring, newCuvette);
  }

  return <Card {...props}>
    <CardHeader className="pb-0 flex flex-row items-center justify-between gap-2">
      {/*<div className="grow flex flex-row items-center gap-2">*/}
        <span>Carousel</span>
      {/*  {isZeroing && <Spinner size="sm" color="warning" />}*/}
      {/*  {isZeroing && <span className="text-warning text-sm">Zeroing...</span>}*/}
      {/*</div>*/}
      <Dropdown className="m-0">
        <DropdownTrigger>
          <Button
            variant="light"
            isIconOnly
            className="m-0"
          >
            <MoreHorizontal />
          </Button>
        </DropdownTrigger>
        <DropdownMenu aria-label="Carousel Options">
          <DropdownItem
            key="manual"
            startContent={useManualPosition ? <Check /> : <></>}
            onPress={() => setUseManualPosition(!useManualPosition)}
          >
            Use Manual Position
          </DropdownItem>
          <DropdownItem
            key="ignoreZeroing"
            startContent={ignoreZeroingStatus ? <Check /> : <></>}
            onPress={() => setIgnoreZeroingStatus(!ignoreZeroingStatus)}
          >
            Ignore Zeroing Status
          </DropdownItem>
        </DropdownMenu>
      </Dropdown>
    </CardHeader>
    <CardBody className="grid grid-cols-5 gap-3">
      {/*<div className="flex flex-col col-span-2 gap-3">*/}

      {/*  <SegmentedPicker*/}
      {/*    selectedIndex={selectedTab}*/}
      {/*    onIndexChange={setSelectedTab}*/}
      {/*    children={[*/}
      {/*      "Analysis", "Calibration"*/}
      {/*    ]}*/}
      {/*    color="primary"*/}
      {/*    className="pb-0"*/}
      {/*    fullWidth*/}
      {/*    variant="bordered"*/}
      {/*  />*/}

      {/*  /!*<CarouselInputs*!/*/}
      {/*  /!*  currentCuvettes={currentCuvettes}*!/*/}
      {/*  /!*  showCalibration={showCalibration}*!/*/}
      {/*  /!*  setCurrentCuvette={(positions) => {*!/*/}
      {/*  /!*    setCuvettePosition(RING.INNER, positions[RING.INNER]);*!/*/}
      {/*  /!*    setCuvettePosition(RING.OUTER, positions[RING.OUTER]);*!/*/}
      {/*  /!*  }}*!/*/}
      {/*  /!*  moveXCuvettes={moveXCuvettes}*!/*/}
      {/*  /!*  triggerZero={triggerZero}*!/*/}
      {/*  /!*  incrementZero={incrementZero}*!/*/}
      {/*  /!*  resetZero={resetZero}*!/*/}
      {/*  /!*  isZeroing={isZeroing}*!/*/}
        {/*/>/*/}
      {/*</div>*/}

      <div className="col-span-5 flex flex-col gap-3 items-center">
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.OUTER)}
          calibrateByDegrees={incrementZero(RING.OUTER)}
          showCalibration={false}
          variant={RING.OUTER}
          disabled={isZeroing}
        />
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.INNER)}
          calibrateByDegrees={incrementZero(RING.INNER)}
          showCalibration={false}
          variant={RING.INNER}
          disabled={isZeroing}
        />
        <div className="w-3/4">
          <CarouselDial
            inner={{current: currentCuvettes[RING.INNER], onClick: onCuvetteClick(RING.INNER)}}
            outer={{current: currentCuvettes[RING.OUTER], onClick: onCuvetteClick(RING.OUTER)}}
          />
        </div>
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.OUTER)}
          calibrateByDegrees={incrementZero(RING.OUTER)}
          showCalibration={true}
          variant={RING.OUTER}
          disabled={isZeroing}
        />
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.INNER)}
          calibrateByDegrees={incrementZero(RING.INNER)}
          showCalibration={true}
          variant={RING.INNER}
          disabled={isZeroing}
        />
        <div className="grid grid-cols-2 gap-3 w-full mt-2">
          <Button color="primary" onPressStart={() => triggerZero(RING.INNER)}>Zero Inner</Button>
          <Button color="secondary" onPressStart={() => triggerZero(RING.OUTER)}>Zero Outer</Button>
        </div>
        <CarouselHallEffects/>
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2
