import React, {useEffect, useState} from "react";
import {Button, Card, CardBody, CardHeader, CardProps, Input} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import {ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, CornerLeftUp, CornerRightUp} from "react-feather";
import LEDRow from "./LEDRow.tsx";
import {toInteger} from "lodash";

export interface CarouselWidgetProps extends CardProps{
}

const clampStep = (step: number) => {
  if (step >= 0 )
    return step % 20 === 0 ? 20 : (step) % 20

  const n = step + (toInteger(step / 20) * -1 + 1) * 20
  return n % 20 === 0 ? 20 : (n) % 20
}

// const CuvvetteStepSize = 40
const nightyDegrees = 5

const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  const [currentStep, setCurrentStep] = useState(1)
  const clampedCurrentStep = clampStep(currentStep)
  const [currentStepInput, setCurrentStepInput] = useState(1)
  const [visableCuvetteInput, setVisableCuvetteInput] = useState(1)
  const [visSpecCuvetteInput, setVisSpecCuvetteInput] = useState(6)

  useEffect(() => {
    setCurrentStepInput(clampedCurrentStep)
    setVisableCuvetteInput(clampedCurrentStep)
    setVisSpecCuvetteInput(clampStep(clampedCurrentStep + nightyDegrees))
  }, [currentStep]);

  const syncCurrentStep = () => {
    if (currentStepInput <= 20 && currentStepInput >= 1)
      setCurrentStep(currentStepInput)
  }

  const moveXSteps = (steps: number) => {
    setCurrentStep(currentStep + steps)
  }

  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
    <CardBody className="grid grid-cols-5 gap-3">
      <div className="flex flex-col col-span-2 gap-2">
        <span>Current Visible Cuvette</span>
        <div className="flex flex-row gap-3">
          <Input
            type="number"
            step="1"
            isInvalid={currentStepInput > 20 || currentStepInput < 1}
            value={currentStepInput.toString()}
            onValueChange={(s) => setCurrentStepInput(Number(s))}
          />
          <Button onClick={syncCurrentStep}>Sync</Button>
        </div>
        <span className="mt-4">Move Visible Cuvette To</span>
        <div className="flex flex-row gap-3">
          <Input
            type="number"
            step="1"
            isInvalid={visableCuvetteInput > 20 || visableCuvetteInput < 1}
            value={visableCuvetteInput.toString()}
            onValueChange={(s) => setVisableCuvetteInput(Number(s))}
          />
          <Button onClick={() => moveXSteps(clampedCurrentStep - clampStep(visableCuvetteInput))}>Go To</Button>
        </div>
        <span className="mt-4">Move Vis Spec Cuvette To</span>
        <div className="flex flex-row gap-3">
          <Input
            type="number"
            step="1"
            isInvalid={visSpecCuvetteInput > 20 || visSpecCuvetteInput < 1}
            value={visSpecCuvetteInput.toString()}
            onValueChange={(s) => setVisSpecCuvetteInput(Number(s))}
          />
          <Button onClick={() => moveXSteps(clampedCurrentStep + nightyDegrees - clampStep(visSpecCuvetteInput))}>Go To</Button>
        </div>
      </div>

      <div className="col-span-3">
        <CarouselDial step={currentStep}/>
      </div>

      <div className="col-span-2 place-self-center">
        <LEDRow/>
      </div>

      <div className="col-span-3 flex flex-row justify-center gap-3 place-self-center">
        <Button isIconOnly onClick={() => setCurrentStep(currentStep - nightyDegrees)}><CornerLeftUp/></Button>
        <Button isIconOnly onClick={() => setCurrentStep(currentStep - 1)}><ChevronsLeft/></Button>
        <Button isIconOnly><ChevronLeft/></Button>
        <Button isIconOnly><ChevronRight/></Button>
        <Button isIconOnly onClick={() => setCurrentStep(currentStep + 1)}><ChevronsRight/></Button>
        <Button isIconOnly onClick={() => setCurrentStep(currentStep + nightyDegrees)}><CornerRightUp/></Button>
      </div>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2