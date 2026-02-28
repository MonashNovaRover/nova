import {Button, Input} from "@nextui-org/react";
import {useState} from "react";
import {Anchor, ArrowDown, ArrowUp, RefreshCcw, Square} from "react-feather";

export interface AnalysisArmControlsProps {
  currentPos: number
  TOFReading: number
  zeroPosition: () => void
  setPosition: (pos: number) => void
  stopAA: () => void
  resetTOF: () => void
}

/**
 * Analysis arm movement controls
 * @constructor
 */
const AnalysisArmControls: React.FC<AnalysisArmControlsProps> = ({currentPos, TOFReading, zeroPosition, setPosition, stopAA, resetTOF} : AnalysisArmControlsProps) => {
  const [stepperTargetInput, setStepperTargetInput] = useState("0")
  const [stepperIncrementInput, setStepperIncrementInput] = useState("5")
  const [distanceTargetInput, setDistanceTargetInput] = useState("50")

  return (
    <div className="flex flex-col justify-evenly">

      <div className="grid grid-cols-3 gap-3 items-end">
        <Button
          color="primary"
          onPressStart={zeroPosition}
          startContent={<Anchor size={16}/>}
        >
          Zero Position
        </Button>
        <Button
          color="warning"
          startContent={<RefreshCcw size={16}/>}
          onPressStart={resetTOF}
        >
          Reset TOF
        </Button>
        <Button
          color="danger"
          startContent={<Square size={16}/>}
          onPressStart={stopAA}
        >
          Stop
        </Button>
      </div>

      <div className="grid grid-cols-2 gap-3 items-end">
        <Input
          type="number"
          label="Move to absoulte position"
          labelPlacement="outside"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">mm</span>
            </div>
          }
          value={stepperTargetInput}
          onValueChange={setStepperTargetInput}
        />
        <Button onPressStart={() => setPosition(parseFloat(stepperTargetInput))}>Go to</Button>
      </div>

      <div className="grid grid-cols-2 gap-3 items-end">
        <Input
          type="number"
          label="Increment position"
          labelPlacement="outside"
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">mm</span>
            </div>
          }
          value={stepperIncrementInput}
          onValueChange={setStepperIncrementInput}
        />
        <div className="grid grid-cols-2 gap-3 content-center">
          <Button onPressStart={() => setPosition(currentPos - parseFloat(stepperIncrementInput))}><ArrowUp/></Button>
          <Button onPressStart={() => setPosition(currentPos + parseFloat(stepperIncrementInput))}><ArrowDown/></Button>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-3 items-center">
        <Input
          type="number"
          label="Move to distance from ground"
          labelPlacement="outside"
          description="Not accurate when TOF reading > 100mm."
          endContent={
            <div className="pointer-events-none flex items-center">
              <span className="text-default-400 text-small">mm</span>
            </div>
          }
          value={distanceTargetInput}
          onValueChange={setDistanceTargetInput}
        />
        <Button onPressStart={() => setPosition(currentPos + TOFReading - parseFloat(distanceTargetInput))}>Go to</Button>
      </div>
    </div>
  );
}

export default AnalysisArmControls