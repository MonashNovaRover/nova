import { useEffect, useEffectEvent, useState } from "react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";

import { RootState } from "../../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { IRosBlcmdInterfacesBlcmdStatus } from "../../../../ros/rosTypes.ts";
import { BLCMD_INDEX } from "../../../../constants.ts";

export const useBLCMDFaults = () => {
  const bifrost = useBifrost({ topic: RosTopic.BLCMD_ERRORS });
  const newStatus = useSelector((state: RootState) => state.blcmdStatusStore.blcmds);
  const [lastStatus, setLastStatus] = useState<string>();
  const [faultMessage, setFaultMessage] = useState<string>();

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]); 

  // The Conditional statement checks if the newStatus is different from the currentStatus. Since it's a list of status
  // objects, we can't compare them directly. We have to convert them to strings and compare the strings.
  const flattenStatus = (status: IRosBlcmdInterfacesBlcmdStatus[]): string => {
    return JSON.stringify(status.sort((a, b) => a.id - b.id))
  }

  // generate an error message depending on the errors received
  const generateErrorMessage = (errors: IRosBlcmdInterfacesBlcmdStatus[]): string => 
    errors.length === 1 ?
      `${BLCMD_INDEX[errors[0].id]} faulted due to short`
      : errors.reduce((p, error)=> p +
        `${BLCMD_INDEX[error.id]} faulted due to ` 
        + (error.gate_fault ? "Gate Fault" : "")
        + (error.overspeed_fault ? "Overspeed Fault," : "")
        + (error.resolver_fault ? "Resolver Fault, " : "")
        + (error.stall_fault ? "Stall Fault, " : "")
      , "").slice(0, -2); // Remove the trailing comma and space


  // renders will occur each time the ros topic updates, 
  // thus this will update the fault message each time a new status is snet
  if (lastStatus !== flattenStatus(newStatus)) {
    const errors = newStatus.filter((status) =>
      status.gate_fault 
      || status.overspeed_fault 
      || status.resolver_fault 
      || status.stall_fault
    );
    if (errors.length > 0) {
      setFaultMessage(generateErrorMessage(errors))
    } else {
      setFaultMessage(undefined)
    }
    setLastStatus(flattenStatus(newStatus))
  }

  useEffectEvent(()=>{
    setLastStatus(flattenStatus(newStatus))
  })

  return faultMessage;
};