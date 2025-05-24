import React, {useEffect, useState} from "react";
import {Button, Slider, Tooltip} from "@nextui-org/react";
import {Power, Square} from "react-feather";
import {isArray} from "lodash";

export interface HeaterControlProps {
  currentHeaterStatus: boolean
  setHeaterStatus: (x : boolean) => void
  targetTemp: number
  setTargetTemp: (x: number) => void
}

/**
 * Controls for turning the heater on/off and adjusting the target temperature
 * @param currentHeaterStatus the current state of the heater (on/off)
 * @param setHeaterStatus request the heater to change state
 * @param targetTemp the current target temperature
 * @param setTargetTemp request a change in target temperature
 * @constructor
 */
const HeaterControl: React.FC<HeaterControlProps> = ({currentHeaterStatus, setHeaterStatus, targetTemp, setTargetTemp}) => {
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
      <Button
        isDisabled
        className={`w-2/3 opacity-100 ${currentHeaterStatus ? "bg-success" : "bg-content3"}`}>
          {currentHeaterStatus ? "POWERED ON" : "POWERED OFF"}
      </Button>
      <Button className="w-1/3 text-h1" color="primary" onPress={() => setHeaterStatus(!currentHeaterStatus)}>
        {currentHeaterStatus ? "STOP HEATER" : "START HEATER"}
        {currentHeaterStatus ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
    </div>
  );

  const targetSlider = (
    <Slider
      value={targetInput}
      onChange={v => isArray(v) ? setTargetInput(v[0]) : setTargetInput(v)}
      onChangeEnd={(v) => isArray(v) ? setTargetTemp(v[0]) : setTargetTemp(v)}
      size="lg"
      classNames={{
        label: "text-medium",
      }}
      color="primary"
      label="Heater Target Temperature"
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
              className="px-1 py-0.5 w-14 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
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
    <div className="flex flex-col gap-3">
      {targetSlider}
      {heatStatus}
    </div>
  );
}

export default HeaterControl;
