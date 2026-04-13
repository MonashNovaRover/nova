import React, {useState} from "react";
import {useGenericStore} from "../../../../hooks/useGenericStore.ts";
import {Input, Tooltip} from "@nextui-org/react";
import { AngleType } from "./Theta360CamWidget.tsx";

export interface heightCalc360CamProps {
  angles: AngleType,
  setAngles:  React.Dispatch<React.SetStateAction<AngleType>>
}

  const HeightCalculator: React.FC<heightCalc360CamProps> = (props) => {

  // local stores for input fields
  const [inputDistance, setInputDistance] = useGenericStore<string>("theta360InputDistance");
  const [inputLow, setInputLow] = useState<string>(props.angles.low.toFixed(2))
  const [inputHigh, setInputHigh] = useState<string>(props.angles.high.toFixed(2))
  

  // Calculate height of landmark
  const radThetaHigh = props.angles.high * Math.PI / 180;
  const radThetaLow = props.angles.low * Math.PI / 180;
  const landmarkHeight = Number(inputDistance) * (Math.tan(radThetaHigh) - Math.tan(radThetaLow));

  // set angles when text inputs change - handled in event handlers in input elements
  // set input text boxes when angles are updated from canvas - handled by reading props for inital state
  // height is calculated on render, render triggered on state change

  // input fields for height calculation
  const distField = (
    <div className="flex flex-row p-1 justify-start">
      <Input
        className="w-3/4"
        label="Distance"
        placeholder="0.0m"
        value={inputDistance}
        onValueChange={setInputDistance}
      />
    </div>
  )
  const lowThetaField = (
    <div className="flex flex-row p-1 justify-start">
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="right"
      >
      <Input
        className="w-3/4"
        label="Lower Angle"
        placeholder="0.0deg"
        value={inputLow}
        onValueChange={setInputLow}
        onKeyDown={(e) => {
          const val = (e.target as HTMLInputElement).value
          if (e.key === "Enter" && !isNaN(Number(val))) {
            props.setAngles({...props.angles, 
              low: Number(val)});
          }
        }}
      />
      </Tooltip>
    </div>
  )
  const highThetaField = (
    <div className="flex flex-row p-1 justify-start">
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="right"
      >
      <Input
        className="w-3/4"
        label="Upper Angle"
        placeholder="0.0deg"
        value={inputHigh}
        onValueChange={setInputHigh}
        onKeyDown={(e) => {
          const val = (e.target as HTMLInputElement).value
          if (e.key === "Enter" && !isNaN(Number(val))) {
            props.setAngles({...props.angles, 
              high: Number(val)});
          }
        }}
      />
      </Tooltip>
    </div>
  )

  return (
    <div className="flex flex-row gap-1 justify-start my-5">
      {distField}
      {lowThetaField}
      {highThetaField}
      <div className="flex flex-row p-1 justify-start my-3.5">
      Height of landmark: {landmarkHeight.toPrecision(2)}m
      </div>
    </div>
  )
}

export default HeightCalculator;