import React, {useEffect, useState} from "react";
import {Button, Card, CardBody, Slider, Tooltip} from "@nextui-org/react";
import {Power, Square} from "react-feather";
import {isArray} from "lodash";

export interface HeaterToggleWidgetProps {
  currentHeaterStatus: boolean
  setHeaterStatus: (x : boolean) => void
  targetTemp: number
  setTargetTemp: (x: number) => void
}

const HeaterToggle: React.FC<HeaterToggleWidgetProps> = ({currentHeaterStatus, setHeaterStatus, targetTemp, setTargetTemp}) => {
  const [targetInput, setTargetInput] = useState<number>(targetTemp)
  const [maxTemp, setMaxTemp] = useState<number>(100)

  useEffect(() => {
    if (targetTemp > maxTemp) {
      setMaxTemp(1.1 * targetTemp);
    }
    setTargetInput(targetTemp)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [targetTemp]);

  const heatStatus = (
    <div className="flex flex-row justify-between gap-5">
      <Card
        className={`w-2/3 ${currentHeaterStatus ? "bg-success" : "bg-content3"}`}>
        <CardBody className="text-center">
          {currentHeaterStatus ? "POWERED ON" : "POWERED OFF"}
        </CardBody>
      </Card>
      <Button className="w-1/3 text-h1 h-12" color="primary" onPress={() => setHeaterStatus(!currentHeaterStatus)}>
        {currentHeaterStatus ? "STOP HEATER" : "START HEATER"}
        {currentHeaterStatus ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
    </div>
  );

  const targetSlider = (
    <Slider
      value={targetInput}
      onChange={v => isArray(v) ? setTargetInput(v[0]) : setTargetInput(v)}
      onChangeEnd={v => isArray(v) ? setTargetTemp(v[0]) : setTargetTemp(v)}
      size="lg"
      classNames={{
        label: "text-medium",
      }}
      color="primary"
      label="Target"
      maxValue={maxTemp}
      minValue={0}
      step={1}
      renderValue={({children, ...props}) => (
        <output {...props}>
          <Tooltip
            className="text-tiny text-default-500 rounded-md"
            content="Press Enter to confirm"
            placement="left"
          >
            <input
              aria-label="Temperature value"
              className="px-1 py-0.5 w-12 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
              type="number"
              value={targetInput}
              onChange={(e: React.ChangeEvent<HTMLInputElement>) => {
                const v = Number(e.target.value);
                setTargetInput(v);
              }}
              onKeyDown={(e: React.KeyboardEvent<HTMLInputElement>) => {
                if (e.key === "Enter" && !isNaN(Number(targetInput))) {
                  setTargetTemp(Number(targetInput));
                }
              }}
            />
          </Tooltip>
        </output>
      )}
    />
  )

  return (
    <div className="flex flex-col gap-6">
      {heatStatus}
      {targetSlider}
    </div>
  );
}

export default HeaterToggle;
