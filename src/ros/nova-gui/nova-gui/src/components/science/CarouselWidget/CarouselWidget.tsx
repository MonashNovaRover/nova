import React, {useState} from "react";
import {Card, CardBody, CardHeader, CardProps, Divider, Switch} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import LEDRow from "./LEDRow.tsx";
import {toInteger} from "lodash";
import CarouselInputs from "./CarouselInputs.tsx";
import CarouselControls from "./CarouselControls.tsx";

export interface CarouselWidgetProps extends CardProps{
}

const clampStep = (step: number) => {
  if (step >= 0 )
    return step % 20

  const n = step + (toInteger(step / 20) * -1 + 1) * 20
  return n % 20
}

const CuvvetteStepSize = 40

const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  const [currentCuvetteRotation, setCurrentCuvetteRotation] = useState(0) // 0-indexed no range
  const currentCuvette = clampStep(currentCuvetteRotation) // 0-indexed between 0-19 inclusive
  const [showCalibration, setShowCalibration] = useState(true)
  const [stepperActive, setStepperActive] = useState(false)

  const moveXSteps = (x: number) => {
    if (x !== 0)
      console.log(`moving the stepper ${x} steps`)
  }

  const moveXCuvettes = (x: number)=> {
    setCurrentCuvetteRotation(currentCuvetteRotation + x)
    moveXSteps(x * CuvvetteStepSize)
  }

  const toggleStepper = (toggle: boolean) => {
    setStepperActive(toggle)
    console.log(`setting stepper to ${toggle}`)
  }

  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
    <CardBody className="grid grid-cols-5 gap-3">
      <div className="flex flex-col col-span-2 gap-3">
        <div className="flex flex-row items-center justify-between gap-5 ">
          <span>Enable Stepper</span>
          <Switch size="lg" isSelected={stepperActive} onValueChange={toggleStepper}/>
        </div>

        <div className="flex flex-row items-center justify-between gap-5 ">
          <span>Calibrate Mode</span>
          <Switch size="lg" isSelected={showCalibration} onValueChange={() => setShowCalibration(!showCalibration)}/>
        </div>

        <Divider className="mt-2"/>

        <CarouselInputs currentCuvette={currentCuvette} showCalibration={showCalibration}
                        setCurrentCuvette={setCurrentCuvetteRotation} moveXCuvettes={moveXCuvettes}/>
      </div>

      <div className="col-span-3">
        <CarouselDial cuvette={currentCuvetteRotation}/>
      </div>

      <div className="col-span-2 place-self-center">
        <LEDRow/>
      </div>

      <CarouselControls currentCuvetteRotation={currentCuvetteRotation} setCurrentCuvetteRotation={setCurrentCuvetteRotation}
                        moveXSteps={moveXSteps} showCalibration={showCalibration}/>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2