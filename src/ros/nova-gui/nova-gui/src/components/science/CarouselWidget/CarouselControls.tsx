import { Button } from "@nextui-org/react";
import React from "react";
import {ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, CornerLeftUp, CornerRightUp} from "react-feather";

export interface CarouselDialProps {
  currentCuvetteRotation: number
  setCurrentCuvetteRotation: (x: number) => void
  moveXSteps: (x: number) => void
  showCalibration: boolean
}

const nightyDegrees = 5

/**
 * A representation of the Carousel that spins so that the current step is at the bottom
 * @param props
 * @param cuvette the current cuvette to display at the bottom (0-indexed)
 * @constructor
 */
const CarouselControls: React.FC<CarouselDialProps> = ({currentCuvetteRotation, setCurrentCuvetteRotation, moveXSteps, showCalibration}) => {
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
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation - nightyDegrees)}><CornerLeftUp/></Button>
      <Button isIconOnly
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation - 2)}><ChevronsLeft/></Button>
      <Button isIconOnly
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation - 1)}><ChevronLeft/></Button>
      <Button isIconOnly
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation + 1)}><ChevronRight/></Button>
      <Button isIconOnly
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation + 2)}><ChevronsRight/></Button>
      <Button isIconOnly
              onClick={() => setCurrentCuvetteRotation(currentCuvetteRotation + nightyDegrees)}><CornerRightUp/></Button>
    </div>
  )


  return showCalibration ? calibratingControls : cuvetteControls
}

export default CarouselControls

