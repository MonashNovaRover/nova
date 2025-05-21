import React, {useEffect, useState} from "react";
import {Button, Input} from "@nextui-org/react";
import {ChevronUp, Search} from "react-feather";

export interface CarouselInputsProps{
  currentCuvette: number // 0-indexed between 0-19 inclusive
  showCalibration: boolean
  setCurrentCuvette: (x: number) => void
  moveXCuvettes : (x: number) => void
}

const relativeVisSpecPos = (x: number) => (x+5) % 20

/**
 * Carousel Inputs component
 * @param currentCuvette current cuvette the camera is facing
 * @param showCalibration whether or not to be in calibration mode
 * @param setCurrentCuvette set the current cuvette
 * @param moveXCuvettes move the carousel x cuvettes
 * @constructor
 */
const CarouselInputs: React.FC<CarouselInputsProps> = ({currentCuvette, showCalibration, setCurrentCuvette, moveXCuvettes}: CarouselInputsProps) => {
  const [currentCuvetteInput, setCurrentCuvetteInput] = useState(currentCuvette + 1) // 1-indexed between 1-20
  const [visibleCuvetteInput, setVisibleCuvetteInput] = useState(currentCuvette + 1) // 1-indexed between 1-20
  const [visSpecCuvetteInput, setVisSpecCuvetteInput] = useState(relativeVisSpecPos(currentCuvette) + 1) // 1-indexed between 1-20

  useEffect(() => {
    setCurrentCuvetteInput(currentCuvette + 1)
    setVisibleCuvetteInput(currentCuvette + 1)
    setVisSpecCuvetteInput(relativeVisSpecPos(currentCuvette) + 1)
  }, [currentCuvette]);

  return (
    <div>
      {showCalibration && <div className="flex flex-col gap-2">
        <span>Current Visible Cuvette</span>
        <div className="flex flex-row gap-3">
          <Input
            startContent={<ChevronUp/>}
            type="number"
            step="1"
            isInvalid={currentCuvetteInput > 20 || currentCuvetteInput < 1}
            value={currentCuvetteInput.toString()}
            onValueChange={(s) => setCurrentCuvetteInput(Number(s))}
          />
          <Button isDisabled={currentCuvetteInput > 20 || currentCuvetteInput < 1}
              onClick={() => setCurrentCuvette(currentCuvetteInput - 1)}>Sync</Button>
        </div>
        <span className="mt-3 mb-1">40 steps = 1 full cuvette</span>
        <span>Move small steps to center the camera view, then sync the current cuvette.</span>
      </div>}

      {!showCalibration && <div className="flex flex-col gap-2">
        <span>Move Visible Cuvette To</span>
        <div className="flex flex-row gap-3">
          <Input
            startContent={<ChevronUp/>}
            type="number"
            step="1"
            isInvalid={visibleCuvetteInput > 20 || visibleCuvetteInput < 1}
            value={visibleCuvetteInput.toString()}
            onValueChange={(s) => setVisibleCuvetteInput(Number(s))}
          />
          <Button isDisabled={visibleCuvetteInput > 20 || visibleCuvetteInput < 1}
                  onClick={() => moveXCuvettes(visibleCuvetteInput - currentCuvette - 1)}>Go To</Button>
        </div>
        <span className="mt-2">Move Vis Spec Cuvette To</span>
        <div className="flex flex-row gap-3">
          <Input
            startContent={<Search/>}
            type="number"
            step="1"
            isInvalid={visSpecCuvetteInput > 20 || visSpecCuvetteInput < 1}
            value={visSpecCuvetteInput.toString()}
            onValueChange={(s) => setVisSpecCuvetteInput(Number(s))}
          />
          <Button isDisabled={visSpecCuvetteInput > 20 || visSpecCuvetteInput < 1}
              onClick={() => moveXCuvettes(visSpecCuvetteInput - currentCuvette - 5 - 1)}>Go To</Button>
        </div>
      </div>}

    </div>
  )
}

export default CarouselInputs