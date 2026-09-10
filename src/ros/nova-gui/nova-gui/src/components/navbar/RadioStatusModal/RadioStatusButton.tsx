import { Button } from "@heroui/react";
import { useState } from "react";
import { Radio } from "react-feather";

import { RadioStatusModal } from "./RadioStatusModal.tsx";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { useRadioMonitor } from "./hooks/useRadioMonitor.ts";
import { RadioConnectionStatus } from "./RadioTypes.ts";
import { useSelector } from "react-redux";
import type { RootState } from "../../../redux/RootState";

const radioConnectionStatusColor: {
  [key: string]: "success" | "warning" | "danger" | "secondary";
} = {
  [RadioConnectionStatus.STRONG]: "success",
  [RadioConnectionStatus.WEAK]: "warning",
  [RadioConnectionStatus.LOST]: "danger",
  [RadioConnectionStatus.ERROR]: "secondary",
};

export function RadioStatusButton() {
  
  const uiActions = useUIActions();
  const [rosTimeout, setRosTimeout] = useState(10000);
  const radioHealth = useRadioMonitor(rosTimeout);
  const isModalOpen = useSelector((state: RootState) => state.uiState.radioStatusModalOpen);

  return (
    <div>
      <Button
        size="sm"
        variant="shadow"
        className="w-32"
        isDisabled={radioHealth === RadioConnectionStatus.STARTING}
        color={radioConnectionStatusColor[radioHealth]}
        onPress={() => uiActions.setRadioStatusModalOpen(true)}
      >
        <Radio className="w-4 h-4" />
        {radioHealth}
      </Button>

      {isModalOpen && <RadioStatusModal
        rosTimeout={rosTimeout}
        setRosTimeout={setRosTimeout}
      />}
    </div>
  );
}
