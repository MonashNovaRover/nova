import { useEffect, useState } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";

import { RootState } from "../../../redux/RootState";
import { useSelector } from "react-redux";
import { IRosCoreBlcmdStatus } from "../../../ros/rosTypes";
import toast from "react-hot-toast";
import { BLCMD_INDEX } from "../../../constants";

export const useBLCMDFaults = () => {
  const bifrost = useBifrost({ topic: RosTopic.BLCMD_ERRORS });
  const [currentStatus, setCurrentStatus] = useState<IRosCoreBlcmdStatus[]>();
  const [faultMessage, setFaultMessage] = useState<string>();

  useEffect(() => {
    bifrost.syncWithTopic();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [bifrost]);

  const newStatus = useSelector(
    (state: RootState) => state.blcmdStatusStore.blcmds
  );

  useEffect(() => {
    // The Conditional statement checks if the newStatus is different from the currentStatus. Since it's a list of status
    // objects, we can't compare them directly. We have to convert them to strings and compare the strings.

    if (
      JSON.stringify(newStatus.sort((a, b) => a.id - b.id)) !==
      JSON.stringify(currentStatus?.sort((a, b) => a.id - b.id))
    ) {
      const errors = newStatus.filter(
        (status) =>
          status.gate_fault ||
          status.overspeed_fault ||
          status.resolver_fault ||
          status.stall_fault
      );
      if (errors.length > 0) {
        errors.forEach((e) => {
          toast.error(generateErrorMessageForFault(e));
        });

        if (errors.length === 1) {
          setFaultMessage(generateErrorMessageForFault(errors[0], true));
        } else {
          setFaultMessage("One or More Motors Errored out");
        }
      } else {
        setFaultMessage(undefined);
      }

      setCurrentStatus(newStatus);
    }
  }, [currentStatus, newStatus]);

  return faultMessage;
};

const generateErrorMessageForFault = (
  error: IRosCoreBlcmdStatus,
  short: boolean = false
) => {
  const motorName = BLCMD_INDEX[error.id];
  if (short) {
    return `${motorName} errored out`;
  }

  let errorMessage = `${motorName} Errored out due to `;
  if (error.gate_fault) {
    errorMessage += "Gate Fault, ";
  }
  if (error.overspeed_fault) {
    errorMessage += "Overspeed Fault, ";
  }
  if (error.resolver_fault) {
    errorMessage += "Resolver fault, ";
  }
  if (error.stall_fault) {
    errorMessage += "Stall fault, ";
  }
  errorMessage = errorMessage.slice(0, -2); // Remove the trailing comma and space
  return errorMessage;
};
