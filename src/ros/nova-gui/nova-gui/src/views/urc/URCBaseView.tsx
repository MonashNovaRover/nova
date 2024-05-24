import React from "react";
import PlatformWidget from "../../components/PlatformWidget/PlatformWidget.tsx";

const URCBaseView: React.FC = () => {
  return <div className="p-3 grid auto-cols-fr grid-cols-3">
    <PlatformWidget></PlatformWidget>
  </div>;
};

export default URCBaseView;
