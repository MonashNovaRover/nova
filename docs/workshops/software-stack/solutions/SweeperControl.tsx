/*
 * SOLUTION for src/components/science/SweeperWidget/SweeperControl.tsx.
 */
import React, {useEffect, useState} from "react";
import {Button, Slider, Tooltip} from "@nextui-org/react";
import {Power, Square} from "react-feather";
import {isArray} from "lodash";

export interface SweeperControlProps {
  controlName: string
  currentStatus: boolean
  setStatus: (x: boolean) => void
  currentEffort: number
  setEffort: (x: number) => void
}

/**
 * Controls for spinning the sweeper up and down, and setting how hard it spins.
 *
 * @param controlName the name shown on the control
 * @param currentStatus whether the sweeper is currently spinning
 * @param setStatus request the sweeper to start/stop
 * @param currentEffort the current effort, as a percentage from -100 to 100
 * @param setEffort request a change in effort
 * @constructor
 */
const SweeperControl: React.FC<SweeperControlProps> = ({controlName, currentStatus, setStatus, currentEffort, setEffort}) => {
  const [effortInput, setEffortInput] = useState<number>(currentEffort)

  useEffect(() => {
    setEffortInput(currentEffort)
  }, [currentEffort]);

  const sweeperStatus = (
    <div className="flex flex-row items-stretch gap-3">
      <Button
        isDisabled
        className={`flex-1 opacity-100 ${currentStatus ? "bg-success" : "bg-content3"}`}>
        {currentStatus ? "SWEEPING" : "STOPPED"}
      </Button>
      <Button
        className="shrink-0 text-h1"
        color="primary"
        onPress={() => setStatus(!currentStatus)}>
        {currentStatus ? `STOP ${controlName.toUpperCase()}` : `START ${controlName.toUpperCase()}`}
        {currentStatus ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
    </div>
  );

  // Bonus task: effort as a number you can type, or drag
  const effortSlider = (
    <Slider
      value={effortInput}
      onChange={v => isArray(v) ? setEffortInput(v[0]) : setEffortInput(v)}
      onChangeEnd={v => isArray(v) ? setEffort(v[0]) : setEffort(v)}
      size="lg"
      classNames={{label: "text-medium"}}
      color="primary"
      fillOffset={0}
      label={`${controlName} Effort`}
      maxValue={100}
      minValue={-100}
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
                setEffortInput(Math.min(100, Math.max(-100, v)));
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
  );

  return (
    <div className="flex flex-col gap-3">
      {effortSlider}
      {sweeperStatus}
    </div>
  );
}

export default SweeperControl;
