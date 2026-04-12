import React, {useState} from "react";
import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import CarouselInputs from "./CarouselInputs.tsx";
import CarouselControls from "./CarouselControls.tsx";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {useCarouselPosition, useCarouselServices} from "./useCarouselBifrost.ts";

export enum RING {
  INNER = 0,
  OUTER = 1,
}
export const RING_NAMES = ["inner", "outer"];
export const NUM_CUVETTES = [15, 24]

export type CuvettePositions = [number, number]

export interface CarouselWidgetProps extends CardProps{
}

/**
 * Widget to control the carousel.
 * @param props
 * @constructor
 */
const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  const [currentBifrostCuvettes, _] = useCarouselPosition()
  const carouselService = useCarouselServices()
  console.log(currentBifrostCuvettes)

  // current cuvette locations both 0 indexed
  // [inner: between [0, 14], outer: between [0, 23]
  const [currentCuvettes, setCurrentCuvettes] = useState<CuvettePositions>([0,0])
  const [selectedTab, setSelectedTab] = useState(0)
  const showCalibration = selectedTab === 1


  const setCuvettePositions = (positions: CuvettePositions) => {
    carouselService(
      {
        names: ["inner_cuvette", "outer_cuvette"],
        positions: positions
      }
    )

    console.log({
      names: ["inner_cuvette", "outer_cuvette"],
      positions: positions
    })

    setCurrentCuvettes(positions)
  }

  const onCuvetteClick = (ring: RING) => (index: number) => {
    setCuvettePositions(
      currentCuvettes.map((val, i) => i == ring ? index : val) as CuvettePositions
    )
  }

  const moveXCuvettes = (ring: RING)=> (x: number) => {
    const newPos = (currentCuvettes[ring] + x + NUM_CUVETTES[ring]) % NUM_CUVETTES[ring]
    setCuvettePositions(
      currentCuvettes.map((val, i) => i == ring ? newPos: val) as CuvettePositions
    )
  }

  const moveXDegrees = (ring: RING)=> (x: number)=> {
    console.log(`moving ring ${ring} ${x} degrees`)
  }

  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
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
          setCurrentCuvette={setCurrentCuvettes}
          moveXCuvettes={moveXCuvettes}
        />
      </div>

      <div className="col-span-3 flex flex-col gap-3">
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.OUTER)}
          moveXSteps={moveXDegrees(RING.OUTER)}
          showCalibration={showCalibration}
          variant={RING.OUTER}
          reverse
        />
        <CarouselControls
          moveXCuvettes={moveXCuvettes(RING.INNER)}
          moveXSteps={moveXDegrees(RING.INNER)}
          showCalibration={showCalibration}
          variant={RING.INNER}
          reverse
        />
        <CarouselDial
          inner={{current: currentBifrostCuvettes[RING.INNER], onClick: onCuvetteClick(RING.INNER)}}
          outer={{current: currentBifrostCuvettes[RING.OUTER], onClick: onCuvetteClick(RING.OUTER)}}
        />
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2