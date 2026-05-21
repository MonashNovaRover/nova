import React from "react";
import PresentationWidgetsContainer from "../../components/science/PresentationWidgets/PresentationWidgetsContainer.tsx";

const URCSciencePresentationView: React.FC = () => {
  return (
    <div className="flex flex-col overflow-hidden" style={{ height: "calc(100vh - 4.05rem)" }}>
      <PresentationWidgetsContainer />
    </div>
  )
};

export default URCSciencePresentationView;
