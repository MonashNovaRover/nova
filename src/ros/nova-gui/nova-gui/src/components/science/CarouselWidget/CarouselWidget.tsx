import React, {useEffect, useState} from "react";
import {Card, CardBody, CardHeader, CardProps, Divider, Switch} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import {toInteger} from "lodash";
import CarouselInputs from "./CarouselInputs.tsx";
import CarouselControls from "./CarouselControls.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {
  IRosScienceInterfacesKilnCommandRequest,
  IRosScienceInterfacesKilnCommandResponse,
} from "../../../ros/rosTypes.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import toast from "react-hot-toast";

export enum RING {
  INNER = 0,
  OUTER = 1,
}

type CuvettePositions = [number, number]

export interface CarouselWidgetProps extends CardProps{
}

const clampStep = (step: number) => {
  if (step >= 0 )
    return step % 20

  const n = step + (toInteger(step / 20) * -1 + 1) * 20
  return n % 20
}

const CuvvetteStepSize = 40

/**
 * Widget to control the carousel.
 * @param props
 * @constructor
 */
const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  const [currentCuvetteRotation, setCurrentCuvetteRotation] = useState(0) // 0-indexed no range
  const currentCuvette = clampStep(currentCuvetteRotation) // 0-indexed between 0-19 inclusive
  const [showCalibration, setShowCalibration] = useState(true)
  const bifrost = useBifrost({service: RosService.CAROUSEL});

  // current cuvette locations both 0 indexed
  // [inner: between [0, 14], outer: between [0, 23]
  const [currentCuvettes, setCurrentCuvettes] = useState<CuvettePositions>([0,0])

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const setCuvettePositions = (positions: CuvettePositions) => {
    // TODO add ROS stuff

    setCurrentCuvettes(positions)
  }

  const onCuvetteClick = (ring: RING) => (index: number) => {
    setCuvettePositions(
      currentCuvettes.map((val, i) => i == ring ? index : val) as CuvettePositions
    )
  }

  const moveXCuvettes = (ring: RING)=> (x: number) => {
    setCuvettePositions(
      currentCuvettes.map((val, i) => i == ring ? val + x : val) as CuvettePositions
    )
  }

  const moveXDegrees = (ring: RING)=> (x: number)=> {
    console.log(`moving ring ${ring} ${x} degrees`)
  }

  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
    <CardBody className="grid grid-cols-5 gap-3">
      <div className="flex flex-col col-span-2 gap-3">

        <div className="flex flex-row items-center justify-between gap-5 ">
          <span>Calibrate Mode</span>
          <Switch size="lg" isSelected={showCalibration} onValueChange={() => setShowCalibration(!showCalibration)}/>
        </div>

        <Divider className="mt-2"/>

        <CarouselInputs
          currentCuvette={currentCuvette}
          showCalibration={showCalibration}
          setCurrentCuvette={setCurrentCuvetteRotation}
          moveXCuvettes={moveXCuvettes}
        />
      </div>

      <div className="col-span-3 flex flex-col gap-3">
        <CarouselDial
          inner={{current: currentCuvettes[RING.INNER], onClick: onCuvetteClick(RING.INNER)}}
          outer={{current: currentCuvettes[RING.OUTER], onClick: onCuvetteClick(RING.OUTER)}}
        />
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
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2