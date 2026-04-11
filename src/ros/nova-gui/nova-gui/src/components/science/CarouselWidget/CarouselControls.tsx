import { Button } from "@nextui-org/react";
import React from "react";
import {ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, CornerLeftUp, CornerRightUp} from "react-feather";
import {RecordCircle, RecordCircleFill} from "react-bootstrap-icons";

export interface CarouselDialProps {
  moveXCuvettes: (x: number) => void,
  moveXSteps: (x: number) => void
  showCalibration: boolean
  variant: 'inner' | 'outer';
}

const nightyDegrees = 5

/**
 * Controls that tell the carousel to move some amount of steps or cuvettes
 * @param moveXCuvettes move the carousel x cuvettes
 * @param moveXSteps move the carousel x steps
 * @param showCalibration whether or not to be in calibration mode
 * @constructor
 */
const CarouselControls: React.FC<CarouselDialProps> = ({moveXCuvettes, moveXSteps, showCalibration, variant}) => {
  const circleIcon = variant == "inner" ?
    <RecordCircle size={24}/> :
    <RecordCircleFill size={24}/>

  const calibratingControls = (
    <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center items-center">
      <Button isIconOnly onPressStart={() => moveXSteps(-20)}>-20</Button>
      <Button isIconOnly onPressStart={() => moveXSteps(-5)}>-5</Button>
      <Button isIconOnly onPressStart={() => moveXSteps(-1)}>-1</Button>
      {circleIcon}
      <Button isIconOnly onPressStart={() => moveXSteps(1)}>+1</Button>
      <Button isIconOnly onPressStart={() => moveXSteps(5)}>+5</Button>
      <Button isIconOnly onPressStart={() => moveXSteps(20)}>+20</Button>
    </div>
  )

  const cuvetteControls = (
    <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center items-center">
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(-nightyDegrees)}><CornerLeftUp/></Button>
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(-2)}><ChevronsLeft/></Button>
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(-1)}><ChevronLeft/></Button>
      {circleIcon}
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(1)}><ChevronRight/></Button>
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(2)}><ChevronsRight/></Button>
      <Button isIconOnly
              onPressStart={() => moveXCuvettes(nightyDegrees)}><CornerRightUp/></Button>
    </div>
  )

  return showCalibration ? calibratingControls : cuvetteControls
}

export default CarouselControls

