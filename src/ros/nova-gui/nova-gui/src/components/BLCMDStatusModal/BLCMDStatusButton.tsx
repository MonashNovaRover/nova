import { Button } from "@nextui-org/react";
import { BLCMDStatusModal } from "./BLCMDStatusModal";
import { ExclamationCircleFill } from "react-bootstrap-icons";
import { useUIActions } from "../../redux/actions/useUIActions";
import { useBLCMDFaults } from "./hooks/useBLCMDFaults";

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
        onClick={() => uiActions.setBlcmdStatusModalOpen(true)}
      >
        <p>{fault ? fault :<p>No Errors</p>}</p>
        <ExclamationCircleFill className="w-4 h-4"/>
      </Button>
      <BLCMDStatusModal></BLCMDStatusModal>
    </div>
  );
};
