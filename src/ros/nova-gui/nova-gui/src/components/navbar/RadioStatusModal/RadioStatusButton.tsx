import { Button } from "@nextui-org/react";
import { RadioStatusModal } from "./RadioStatusModal.tsx";
import { Radio } from "react-feather";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { useRadioMonitor } from "./hooks/useRadioMonitor.ts";
import { RadioConnectionStatus } from "./RadioTypes.ts";

const radioConnectionStatusColor: {
  [key: string]: "success" | "warning" | "danger";
} = {
  [RadioConnectionStatus.STRONG]: "success",
  [RadioConnectionStatus.WEAK]: "warning",
  [RadioConnectionStatus.LOST]: "danger",
  [RadioConnectionStatus.ERROR]: "danger",
};

export function RadioStatusButton() {

  const uiActions = useUIActions();
  const radioHealth = useRadioMonitor();

  return (
    <div>
      <Button
        size="sm"
        variant="shadow"
        isDisabled={radioHealth === RadioConnectionStatus.STARTING}
        color={radioConnectionStatusColor[radioHealth]}
        onPress={() => uiActions.setRadioStatusModalOpen(true)}
      >
        <Radio className="w-4 h-4" />
        {radioHealth.toString()}
      </Button>
      <RadioStatusModal />
    </div>
  );
}
