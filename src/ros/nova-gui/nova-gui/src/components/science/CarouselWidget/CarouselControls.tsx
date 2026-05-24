import { Button } from "@nextui-org/react";
import React from "react";
import {ChevronLeft, ChevronRight} from "react-feather";
import {RecordCircle, RecordCircleFill} from "react-bootstrap-icons";
import {RING} from "./CarouselWidget.tsx";

export interface CarouselDialProps {
  moveXCuvettes: (x: number) => void,
  calibrateByDegrees: (x: number) => void
  showCalibration: boolean
  variant: RING;
  reverse?: boolean
  disabled?: boolean
}

const degrees = [1, 5, 20]

/**
 * Controls that tell the carousel to move some amount of steps or cuvettes
 * @param moveXCuvettes move the carousel x cuvettes
 * @param moveXSteps move the carousel x steps
 * @param showCalibration whether or not to be in calibration mode
 * @constructor
 */
const CarouselControls: React.FC<CarouselDialProps> = ({moveXCuvettes, calibrateByDegrees, showCalibration, variant, reverse, disabled}) => {
  const reverseNum = reverse ? -1 : 1

  const circleIcon = variant == RING.INNER ?
    <RecordCircle size={30}/> :
    <RecordCircleFill size={30}/>

  const calibratingControls = (
    <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center items-center">
      {degrees.slice().reverse().map(val =>
        <Button isIconOnly isDisabled={disabled} onPressStart={() => calibrateByDegrees(-1 * val * reverseNum)}>-{val}°</Button>
      )}
      {circleIcon}
      {degrees.map(val =>
        <Button isIconOnly isDisabled={disabled} onPressStart={() => calibrateByDegrees(val * reverseNum)}>+{val}°</Button>
      )}
    </div>
  )

  const cuvetteControls = (
    <div className="flex flex-row justify-center gap-3 place-self-center items-center">
      <Button isIconOnly isDisabled={disabled} size="lg" color="primary"
              onPressStart={() => moveXCuvettes(-1 * reverseNum)}><ChevronLeft/></Button>
      {circleIcon}
      <Button isIconOnly isDisabled={disabled} size="lg"
              onPressStart={() => moveXCuvettes(1 * reverseNum)}><ChevronRight/></Button>
    </div>
  )

  return showCalibration ? calibratingControls : cuvetteControls
}

export default CarouselControls

