import React from "react";
import PlatformWidget from "../../components/PlatformWidget/PlatformWidget.tsx";

const URCBaseView: React.FC = () => {
  return <div className="p-3 grid auto-cols-fr grid-cols-2">
    <PlatformWidget></PlatformWidget>
  </div>;
};

export default URCBaseView;
