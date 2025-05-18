import { Button } from "@nextui-org/react";
import React from "react";
import {ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, CornerLeftUp, CornerRightUp} from "react-feather";

export interface CarouselDialProps {
  moveXCuvettes: (x: number) => void,
  moveXSteps: (x: number) => void
  showCalibration: boolean
}

const nightyDegrees = 5

/**
 * Controls that tell the carousel to move some amount of steps or cuvettes
 * @param moveXCuvettes move the carousel x cuvettes
 * @param moveXSteps move the carousel x steps
 * @param showCalibration whether or not to be in calibration mode
 * @constructor
 */
const CarouselControls: React.FC<CarouselDialProps> = ({moveXCuvettes, moveXSteps, showCalibration}) => {
  const calibratingControls = (
    <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center">
      <Button isIconOnly onClick={() => moveXSteps(-20)}>-20</Button>
      <Button isIconOnly onClick={() => moveXSteps(-5)}>-5</Button>
      <Button isIconOnly onClick={() => moveXSteps(-1)}>-1</Button>
      <Button isIconOnly onClick={() => moveXSteps(1)}>+1</Button>
      <Button isIconOnly onClick={() => moveXSteps(5)}>+5</Button>
      <Button isIconOnly onClick={() => moveXSteps(20)}>+20</Button>
    </div>
  )

  const cuvetteControls = (
    <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center">
      <Button isIconOnly
              onClick={() => moveXCuvettes(-nightyDegrees)}><CornerLeftUp/></Button>
      <Button isIconOnly
              onClick={() => moveXCuvettes(-2)}><ChevronsLeft/></Button>
      <Button isIconOnly
              onClick={() => moveXCuvettes(-1)}><ChevronLeft/></Button>
      <Button isIconOnly
              onClick={() => moveXCuvettes(1)}><ChevronRight/></Button>
      <Button isIconOnly
              onClick={() => moveXCuvettes(2)}><ChevronsRight/></Button>
      <Button isIconOnly
              onClick={() => moveXCuvettes(nightyDegrees)}><CornerRightUp/></Button>
    </div>
  )

  return showCalibration ? calibratingControls : cuvetteControls
}

export default CarouselControls

