import React, {useCallback, useEffect, useState} from "react";
import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Input, Tooltip} from "@nextui-org/react";

export interface heightCalc360CamProps {
  angles: number[],
  setAngles:  React.Dispatch<React.SetStateAction<number[]>>
}

  const HeightCalculator: React.FC<heightCalc360CamProps> = (props) => {

  const [inputDistance, setInputDistance] = useGenericStore<string>("theta360InputDistance");
  const [inputThetaHigh, setInputThetaHigh] = useState<string>("");
  const [inputThetaLow, setInputThetaLow] = useState<string>("");
  const [landmarkHeight, setLandmarkHeight] = useState<number>(0);

// Calculate height of landmark
    useEffect(() => {
    const radThetaHigh = props.angles[0] * Math.PI / 180;
    const radThetaLow = props.angles[1] * Math.PI / 180;
    (setLandmarkHeight(Number(inputDistance) * (Math.tan(radThetaHigh) - Math.tan(radThetaLow))));
  }, [setLandmarkHeight, inputDistance, inputThetaHigh, inputThetaLow]);

  // set angles when text inputs change
    const typeAngle = useCallback(() => {
    props.setAngles([Number(inputThetaHigh), Number(inputThetaLow)])
    }, [inputThetaHigh, inputThetaLow])

  // set input text boxes when angles are updated from canvas
    useEffect(() => {
      setInputThetaHigh(props.angles[0].toFixed(2));
      setInputThetaLow(String(props.angles[1].toFixed(2)));
    }, [props.angles])

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
        value={inputThetaLow}
        onValueChange={setInputThetaLow}
        onKeyDown={(e) => {
          if (e.key === "Enter" && !isNaN(Number(inputThetaLow))) {
            typeAngle();
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
        value={inputThetaHigh}
        onValueChange={setInputThetaHigh}
        onKeyDown={(e) => {
          if (e.key === "Enter" && !isNaN(Number(inputThetaHigh))) {
            typeAngle();
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