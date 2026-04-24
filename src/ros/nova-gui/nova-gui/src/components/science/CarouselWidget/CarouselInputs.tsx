import React, {useState} from "react";
import {Button, Input} from "@nextui-org/react";
import {Anchor, Minus, Plus, RotateCcw} from "react-feather";
import {CuvettePositions, RING} from "./CarouselWidget.tsx";

export interface CarouselInputsProps{
  currentCuvettes: CuvettePositions
  showCalibration: boolean
  setCurrentCuvette: (positions: CuvettePositions) => void
  moveXCuvettes : (ring: RING) => (index: number) => void
  triggerZero: () => void
  incrementZero: (amount: number) => void
  resetZero: () => void
  isZeroing: boolean
}

/**
 * Carousel Inputs component
 * @param currentCuvette current cuvette the camera is facing
 * @param showCalibration whether or not to be in calibration mode
 * @param setCurrentCuvette set the current cuvette
 * @param moveXCuvettes move the carousel x cuvettes
 * @param triggerZero trigger the zeroing sequence
 * @param incrementZero increment the zero position by a given amount
 * @param resetZero reset the zero offset
 * @param isZeroing whether the carousel is currently zeroing
 * @constructor
 */
const CarouselInputs: React.FC<CarouselInputsProps> = ({
  showCalibration,
  triggerZero,
  incrementZero,
  resetZero,
  isZeroing
}: CarouselInputsProps) => {
  const [incrementValue, setIncrementValue] = useState<string>("1");

  const handleIncrement = (multiplier: number) => {
    const value = parseFloat(incrementValue);
    if (!isNaN(value)) {
      incrementZero(value * multiplier);
    }
  };

  return (
    <div className="flex flex-col gap-3">
      {showCalibration && (
        <>
          <Button
            color="secondary"
            startContent={<Anchor size={16}/>}
            onClick={triggerZero}
            isDisabled={isZeroing}
            isLoading={isZeroing}
          >
            {isZeroing ? "Zeroing..." : "Zero"}
          </Button>

          <div className="flex flex-col gap-2 mt-2">
            <span className="text-sm text-default-500">Fine Calibration (degrees)</span>
            <div className="flex flex-row gap-2 items-center">
              <Button
                size="sm"
                isIconOnly
                variant="bordered"
                onClick={() => handleIncrement(-1)}
                isDisabled={isZeroing}
              >
                <Minus size={14}/>
              </Button>
              <Input
                type="number"
                size="sm"
                step="0.1"
                value={incrementValue}
                onValueChange={setIncrementValue}
                className="w-20"
                isDisabled={isZeroing}
              />
              <Button
                size="sm"
                isIconOnly
                variant="bordered"
                onClick={() => handleIncrement(1)}
                isDisabled={isZeroing}
              >
                <Plus size={14}/>
              </Button>
            </div>
            <Button
              size="sm"
              variant="flat"
              color="warning"
              startContent={<RotateCcw size={14}/>}
              onClick={resetZero}
              isDisabled={isZeroing}
            >
              Reset Zero Offset
            </Button>
          </div>
        </>
      )}
    </div>
  )
}

export default CarouselInputs
