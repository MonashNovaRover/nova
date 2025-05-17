import React, {useEffect, useState} from "react";
import {useGenericStore} from "../../hooks/useGenericStore.ts";
import {Input} from "@nextui-org/react";

const HeightCalculator: React.FC = ()=> {

  const [inputDistance, setInputDistance] = useGenericStore<string>("theta360InputDistance");
  const [inputThetaHigh, setInputThetaHigh] = useState<string>("");
  const [inputThetaLow, setInputThetaLow] = useState<string>("");
  const [landmarkHeight, setLandmarkHeight] = useState<number>(0);

// Calculate height of landmark
  useEffect(() => {
    const radThetaHigh = Number(inputThetaHigh) * Math.PI / 180;
    const radThetaLow = Number(inputThetaLow) * Math.PI / 180;
    (setLandmarkHeight(Number(inputDistance) * (Math.tan(radThetaHigh) - Math.tan(radThetaLow))));
  }, [setLandmarkHeight, inputDistance, inputThetaHigh, inputThetaLow]);

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
      <Input
        className="w-3/4"
        label="Lower Angle"
        placeholder="0.0deg"
        value={inputThetaLow}
        onValueChange={setInputThetaLow}
      />
    </div>
  )
  const highThetaField = (
    <div className="flex flex-row p-1 justify-start">
      <Input
        className="w-3/4"
        label="Upper Angle"
        placeholder="0.0deg"
        value={inputThetaHigh}
        onValueChange={setInputThetaHigh}
      />
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