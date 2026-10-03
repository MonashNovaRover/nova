import React, {useState} from "react";
import {Button, Slider} from "@nextui-org/react";
import {isArray} from "lodash";

export interface SweeperControlProps {
  controlName: string
  currentStatus: boolean
  setStatus: (x: boolean) => void
}

/**
 * Controls for spinning the sweeper up and down.
 *
 * @param controlName the name shown on the control
 * @param currentStatus whether the sweeper is currently spinning
 * @param setStatus request the sweeper to start/stop
 * @constructor
 */
const SweeperControl: React.FC<SweeperControlProps> = ({controlName, currentStatus, setStatus}) => {
  // TODO bonus: lift this out into props, and send it with the command
  const [effort, setEffort] = useState<number>(100);

  const effortSlider = (
    <Slider
      isDisabled
      value={effort}
      onChange={v => setEffort(isArray(v) ? v[0] : v)}
      size="lg"
      classNames={{label: "text-medium"}}
      color="primary"
      fillOffset={0}
      label={`${controlName} Effort`}
      maxValue={100}
      minValue={-100}
      step={1}
    />
  );

  return (
    <div className="flex flex-col gap-3">
      {effortSlider}
      {/* TODO 4.2: replace this with a SWEEPING/STOPPED pill and a real START/STOP button */}
      <Button color="primary" onPress={() => setStatus(!currentStatus)}>
        TODO: make me a real toggle (currently {currentStatus ? "on" : "off"})
      </Button>
    </div>
  );
}

export default SweeperControl;
