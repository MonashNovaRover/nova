import ReactApexChart from "react-apexcharts";
import {ApexOptions} from "apexcharts";
import {Button, Card, CardBody, CardHeader, Input, Switch} from "@nextui-org/react";
import AnalysisArmDiagram from "./AnalysisPlatformDiagram.tsx";
import {useState} from "react";
import {number} from "framer-motion";

export interface AnalysisArmControlsProps {
  currentStepperPos: number
  moveStepperTo: (number) => void
  moveToDistance: (number) => void
}

/**
 * Analysis arm movement controls
 * @constructor
 */
const AnalysisArmControls: React.FC<AnalysisArmControlsProps> = ({currentStepperPos, moveStepperTo, moveToDistance} : AnalysisArmControlsProps) => {
  const [stepperTargetInput, setStepperTargetInput] = useState("0")
  const [stepperIncrementInput, setStepperIncrementInput] = useState("5")
  const [distanceTargetInput, setDistanceTargetInput] = useState("50")

  return (
    <div className="flex flex-col gap-6">
      <div className="grid grid-cols-3 gap-3 content-center">
        <p className="content-center">Go to abs step</p>
        <Input
          type="number"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">steps</span>
            </div>
          }
          value={stepperTargetInput}
          onValueChange={setStepperTargetInput}
        />
        <Button onPressStart={() => moveStepperTo(parseInt(stepperIncrementInput))}>Go to</Button>
      </div>

      <div className="grid grid-cols-3 gap-3 content-center">
        <Button onPressStart={() => moveStepperTo(currentStepperPos + parseInt(stepperIncrementInput))}>Move down</Button>
        <Input
          type="number"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">steps</span>
            </div>
          }
          value={stepperIncrementInput}
          onValueChange={setStepperIncrementInput}
        />
        <Button onPressStart={() => moveStepperTo(currentStepperPos - parseInt(stepperIncrementInput))}>Move up</Button>
      </div>

      <div className="grid grid-cols-3 gap-3 content-center">
        <p className="content-center">Go to distance from ground</p>
        <Input
          type="number"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">mm</span>
            </div>
          }
          value={distanceTargetInput}
          onValueChange={setDistanceTargetInput}
        />
        <Button onPressStart={() => moveToDistance(parseInt(distanceTargetInput))}>Go to</Button>
      </div>
    </div>
  );
}

export default AnalysisArmControls