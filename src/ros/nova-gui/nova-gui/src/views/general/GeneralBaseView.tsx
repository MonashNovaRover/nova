import React, { useEffect } from "react";
import { useBifrost } from "../../redux/actions/useBifrostAction";
import { RosTopics } from "../../ros/rosTopics";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";

const GeneralBaseView: React.FC = () => {
  const bifrost = useBifrost(RosTopics.DEMO_TOPIC);
  const demoState = useSelector((state: RootState) => state.demo);

  useEffect(() => {
    bifrost.syncWithRover();
  }, []);
  if (!demoState) return;
  return (
    <div>
      <div>Info: {demoState.data ?? ""}</div>
    </div>
  );
};

export default GeneralBaseView;
