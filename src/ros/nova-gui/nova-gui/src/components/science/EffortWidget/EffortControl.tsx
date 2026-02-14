import React, {useEffect, useState} from "react";
import {Button, Slider, Tooltip} from "@nextui-org/react";
import {Power, Square} from "react-feather";
import {isArray} from "lodash";

export interface EffortControlProps {
  controlName: string
  currentStatus: boolean
  setStatus: (x : boolean) => void
  currentEffort: number
  setEffort: (x: number) => void
}

/**
 * Controls for turning the effort system on/off and adjusting the effort level (repurposed from HeaterControl.tsx)
 * @param controlName the name of the effort system
 * @param currentStatus the current state of the effort system (on/off)
 * @param setStatus request the effort system to change state
 * @param currentEffort the current effort
 * @param setEffort request a change in effort
 * @constructor
 */
const EffortControl: React.FC<EffortControlProps> = ({controlName, currentStatus, setStatus, currentEffort, setEffort}) => {
  const [effortInput, setTargetInput] = useState<number>(currentEffort)

  useEffect(() => {
    setTargetInput(currentEffort)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [currentEffort]);

const systemStatus = (
  <div className="flex flex-row items-stretch gap-3">
    <Button
      isDisabled
      className={`flex-1 opacity-100 ${currentStatus ? "bg-success" : "bg-content3"}`}>
      {currentStatus ? "POWERED ON" : "POWERED OFF"}
    </Button>
    <Button
      className="shrink-0 text-h1"
      color="primary"
      onPress={() => setStatus(!currentStatus)}>
      {currentStatus ? `STOP ${controlName.toUpperCase()}` : `START ${controlName.toUpperCase()}`}
      {currentStatus ? <Square size="15" fill="white" /> : <Power size="15" />}
    </Button>
  </div>
);

  const targetSlider = (
    <Slider
      value={effortInput}
      onChange={v => isArray(v) ? setTargetInput(v[0]) : setTargetInput(v)}
      onChangeEnd={(v) => isArray(v) ? setEffort(v[0]) : setEffort(v)}
      size="lg"
      classNames={{
        label: "text-medium",
      }}
      color="primary"
      label={`${controlName}`}
      maxValue={100}
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
              aria-label="Effort value"
              className="px-1 py-0.5 w-14 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
              type="number"
              value={effortInput}
              onChange={(e: React.ChangeEvent<HTMLInputElement>) => {
                const v = Number(e.target.value);
                setTargetInput(Math.min(100, Math.max(0, v)));
              }}
              onKeyDown={(e: React.KeyboardEvent<HTMLInputElement>) => {
                if (e.key === "Enter" && !isNaN(Number(effortInput))) {
                  setEffort(Number(effortInput));
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
      {systemStatus}
    </div>
  );
}

export default EffortControl;
