import { Card, CardHeader, CardBody } from "@nextui-org/react";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/useBifrostAction";
import { RosTopics } from "../../ros/topics/rosTopics";

export const PoseDataWidget: React.FC = () => {
  const bifrost = useBifrost(RosTopics.POSE);
  const poseStore = useSelector((state: RootState) => state.poseStore);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <Card className="w-40 m-4">
      <CardHeader>Pose Data</CardHeader>
      <CardBody>
        <div className="">
          <h3>Position</h3>
          X: {poseStore.position.x}
          Y: {poseStore.position.y}
          Z: {poseStore.position.z}
        </div>
        <div className="">
          <h3>Position</h3>
          X: {poseStore.orientation.x}
          Y: {poseStore.orientation.y}
          Z: {poseStore.orientation.z}
          W: {poseStore.orientation.w}
        </div>
      </CardBody>
    </Card>
  );
};
