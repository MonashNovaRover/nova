import React from "react";
import MicroscopeComponent from "../../components/CameraComponent/MicroscopeComponent";
import TOFHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight";

const ARCBaseView: React.FC = () => {
  return (
    <div>
      <MicroscopeComponent/>
      <TOFHeight />
    </div>
  );
};

export default ARCBaseView;
