import { useState } from "react";
import { Button } from "@nextui-org/react";
import { CartographerGoalModal } from "./CartographerGoalModal.tsx";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
// import { useSelector } from "react-redux";
// import { RootState } from "../../../../redux/RootState.ts";
// import { IRosNovaInterfacesStatusConst } from "../../../../ros/rosTypes.ts";

export const GoalPublisher = () => {
  const [modalOpen, setModalOpen] = useState(false);
  const cancelBifrost = useBifrost({ service: RosService.CANCEL_NAVIGATION });
  // const autoStatus = useSelector((state: RootState) => state.autoStatus.status);
  // const autoStatusValue = Number(autoStatus);
  // const canCancel = autoStatusValue === IRosNovaInterfacesStatusConst.TRAVERSING
  //   || autoStatusValue === IRosNovaInterfacesStatusConst.SEARCHING;

  const onCancelNavigation = () => {
    cancelBifrost.callService({}, {
      successToastMessage: "Navigation cancel requested",
      errorToastMessage: "Failed to cancel navigation",
    });
  };

  return (
    <>
      <div className="flex w-full gap-2">
        <Button
          variant="shadow"
          fullWidth
          onClick={() => setModalOpen(true)}
        >
          Publish Goals
        </Button>
        <Button
          variant="flat"
          color="danger"
          fullWidth
          // isDisabled={!canCancel}
          onClick={onCancelNavigation}
        >
          Cancel
        </Button>
      </div>
      <CartographerGoalModal
        isOpen={modalOpen}
        onClose={() => setModalOpen(false)}
      />
    </>
  );
};