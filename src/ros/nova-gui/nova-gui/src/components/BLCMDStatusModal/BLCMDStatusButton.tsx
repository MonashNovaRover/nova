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
        className="px-40"
        onClick={() => uiActions.setBlcmdStatusModalOpen(true)}
      >
        <ExclamationCircleFill className="w-4 h-4" />
        {fault}
      </Button>
      <BLCMDStatusModal></BLCMDStatusModal>
    </div>
  );
};
