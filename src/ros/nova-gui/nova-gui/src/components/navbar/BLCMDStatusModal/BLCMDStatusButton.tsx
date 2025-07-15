import { Button } from "@nextui-org/react";
import { BLCMDStatusModal } from "./BLCMDStatusModal.tsx";
import { ExclamationCircleFill } from "react-bootstrap-icons";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { useBLCMDFaults } from "./hooks/useBLCMDFaults.ts";

export const BLCMDStatusButton = () => {
  const uiActions = useUIActions();

  const fault = useBLCMDFaults();

  return (
    <div>
      <Button
        isIconOnly={!fault}
        size="sm"
        variant="solid"
        color={fault ? "danger" : "default"}
        className={"w-80 flex flex-row justify-center space-x-2"}
        onPress={() => uiActions.setBlcmdStatusModalOpen(true)}
      >
        {fault ? fault :<p>No Errors</p>}
        <ExclamationCircleFill className="w-4 h-4"/>
      </Button>
      <BLCMDStatusModal/>
    </div>
  );
};
