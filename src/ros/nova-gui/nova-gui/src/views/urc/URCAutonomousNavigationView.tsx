import React from "react";
import { Cartographer } from "../../components/maps/Cartographer/Cartographer";
import AutoStatus from "../../components/auto/Cartographer/components/AutoStatus";
import { GoalPublisher } from "../../components/maps/Cartographer/components/GoalPublisher";

const URCAutonomousNavigationView: React.FC = () => {
  return (
    <div className="w-full h-[92.1vh]">
      <Cartographer
        pointLabels={[
          { key: 0, text: "GNSS" },
          { key: 1, text: "AR Tag" },
          { key: 2, text: "Object" },
        ]}
        bottomOverlayComponents={[
          <AutoStatus key="auto-status" />,
          <GoalPublisher key="goal-publisher" />,
        ]}
      />
    </div>
  );
};

export default URCAutonomousNavigationView;
