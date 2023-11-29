import React, { useContext, useEffect } from "react";
import { RosTopics } from "../../ros/rosTopics";
import { useBifrost } from "../../redux/actions/useBifrostAction";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { RosContext } from "../../RosRoot";

const URCBaseView: React.FC = () => {
  const bifrost = useBifrost(RosTopics.BLCMD_STATUS);
  const blcmdStore = useSelector((state: RootState) => state.blcmdStore);

  useEffect(()=>{
    bifrost.syncWithRover()
  },[])

  if(!blcmdStore) return <>Loading</>;
  return (
    <div>
      <div>BLCMD ID: {blcmdStore.id}</div>
      <div>Gate Fault: {blcmdStore.gate_fault}</div>
      <div>Stall Fault: {blcmdStore.stall_fault}</div>
      <div>Resolver Fault: {blcmdStore.resolver_fault}</div>
      <div>Overspeed Fault: {blcmdStore.overspeed_fault}</div>
    </div>
  );
};

export default URCBaseView;
