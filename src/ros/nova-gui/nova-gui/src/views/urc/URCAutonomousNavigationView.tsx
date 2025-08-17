import React from "react";
import { Cartographer } from "../../components/maps/Cartographer/Cartographer";
import { GoalPublisher } from "../../components/maps/Cartographer/components/GoalPublisher";
import AutoStatus from "../../components/auto/Cartographer/components/AutoStatus";

const URCAutonomousNavigationView: React.FC = () => {
  return (
    <div className="w-full overflow-hidden" style={{ height: "calc(100vh - 4.01em)" }}>
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
