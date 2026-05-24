import React, {useCallback, useEffect, useMemo, useState} from "react";
import {Button, Card, CardBody, CardHeader, CardProps, Chip, Dropdown, DropdownItem, DropdownMenu, DropdownTrigger
} from "@nextui-org/react";
import {Check, MoreHorizontal, Trash2} from "react-feather";
import CarouselDial, {PumpedCuvettes} from "./CarouselDial.tsx";
import CarouselControls from "./CarouselControls.tsx";
import {
  useCarouselFeedback,
  useCarouselSetPosition,
  useCarouselIncrementZero,
  useCarouselZero
} from "./useCarouselBifrost.ts";
import {CarouselHallEffects} from "./CarouselHallEffects.tsx";
import {useCarouselPosition} from "./CarouselPositionContext.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

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
  const { triggerZero: triggerZeroService } = useCarouselZero();
  const { incrementZero } = useCarouselIncrementZero();

  // Manual position override state
  const [useManualPosition, setUseManualPosition] = useState(false);
  const [manualPositions, setManualPositions] = useState<CuvettePositions>([0, 0]); // Current Degrees
  const [ignoreZeroingStatus, setIgnoreZeroingStatus] = useState(true);

  // Track which cuvettes have been pumped into
  const [pumpedCuvettes, setPumpedCuvettes] = useGenericStore<PumpedCuvettes>("pumpedCuvettes");

  const clearPumpedStatus = () => {
    setPumpedCuvettes({ inner: [], outer: [] });
  };

  const hasPumpedCuvettes = pumpedCuvettes.inner.length > 0 || pumpedCuvettes.outer.length > 0;

  // Context for sharing position with other components (e.g., UVVisSpec for graph naming)
  const carouselContext = useCarouselPosition();

  // Derive current cuvette positions from feedback positions (in degrees)
  const currentCuvettes: CuvettePositions = useMemo(() => [
    degreesToCuvette(RING.INNER, useManualPosition ? manualPositions[RING.INNER] : innerFeedback.position),
    degreesToCuvette(RING.OUTER, useManualPosition ? manualPositions[RING.OUTER] : outerFeedback.position)
  ], [useManualPosition, manualPositions, innerFeedback, outerFeedback]);

  const triggerZero = useCallback((ring: RING) => {
    // Set manual position to 0 for the ring being zeroed
    setManualPositions(prev => [
      ring === RING.INNER ? 0 : prev[RING.INNER],
      ring === RING.OUTER ? 0 : prev[RING.OUTER],
    ]);
    // Call the bifrost zero service
    triggerZeroService(ring);
  }, [triggerZeroService])

  // Check if either ring is zeroing
  const isZeroing = ignoreZeroingStatus ? false : (innerFeedback.zeroing || outerFeedback.zeroing);

  // Check if either ring is moving
  const isMoving = innerFeedback.is_moving || outerFeedback.is_moving;

  // Determine status for pill display (zeroing takes priority)
  const statusLabel = isZeroing ? "Zeroing" : isMoving ? "Moving" : "Idle";
  const isActive = isZeroing || isMoving;

  // Sync current cuvette positions to context for use by other components
  useEffect(() => {
    carouselContext?.setPositions(
      currentCuvettes[RING.INNER],
      currentCuvettes[RING.OUTER]
    );
  }, [currentCuvettes, carouselContext]);

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
        <span>Carousel</span>
        <Chip
          size="sm"
          color={isActive ? "warning" : "default"}
          variant={isActive ? "flat" : "bordered"}
        >
          {statusLabel}
        </Chip>
      <div className="flex flex-row items-center gap-1">
        <Button
          size="sm"
          variant="light"
          onPress={clearPumpedStatus}
          isDisabled={!hasPumpedCuvettes}
          startContent={<Trash2 size={16} />}
        >
          Clear Filled
        </Button>
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
      </div>
    </CardHeader>
    <CardBody className="flex flex-col gap-3">
      <div className="grid grid-cols-2 w-full gap-3">
        <Button className="w-full" color="primary" onPress={() => triggerZero(RING.INNER)}>Zero Inner</Button>
        <Button className="w-full" color="secondary" onPress={() => triggerZero(RING.OUTER)}>Zero Outer</Button>
      </div>
      <div className="flex flex-col gap-3 items-center w-full">
        <CarouselHallEffects/>
      </div>

      <div className="flex flex-col gap-3 items-center mt-3">
        <div className="grid grid-cols-2 w-full gap-3">
          <CarouselControls
            moveXCuvettes={moveXCuvettes(RING.INNER)}
            calibrateByDegrees={incrementZero(RING.INNER)}
            showCalibration={false}
            variant={RING.INNER}
            disabled={isZeroing}
          />
          <CarouselControls
            moveXCuvettes={moveXCuvettes(RING.OUTER)}
            calibrateByDegrees={incrementZero(RING.OUTER)}
            showCalibration={false}
            variant={RING.OUTER}
            disabled={isZeroing}
          />
        </div>
        <CarouselDial
          inner={{current: currentCuvettes[RING.INNER], onClick: onCuvetteClick(RING.INNER)}}
          outer={{current: currentCuvettes[RING.OUTER], onClick: onCuvetteClick(RING.OUTER)}}
          pumpedCuvettes={pumpedCuvettes}
        />
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
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2
